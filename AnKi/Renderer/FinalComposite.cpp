// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Renderer/FinalComposite.h>
#include <AnKi/Renderer/Renderer.h>
#include <AnKi/Renderer/Bloom.h>
#include <AnKi/Renderer/Scale.h>
#include <AnKi/Renderer/Tonemapping.h>
#include <AnKi/Renderer/LightShading.h>
#include <AnKi/Renderer/GBuffer.h>
#include <AnKi/Renderer/Dbg.h>
#include <AnKi/Renderer/Ssr.h>
#include <AnKi/Renderer/DownscaleBlur.h>
#include <AnKi/Renderer/UiStage.h>
#include <AnKi/Renderer/MotionVectors.h>
#include <AnKi/Util/Logger.h>
#include <AnKi/Core/ConfigSet.h>

namespace anki {

FinalComposite::FinalComposite(Renderer* r)
	: RendererObject(r)
{
}

FinalComposite::~FinalComposite()
{
}

Error FinalComposite::initInternal(const ConfigSet& config)
{
	ANKI_ASSERT("Initializing PPS");

	ANKI_CHECK(loadColorGradingTextureImage("EngineAssets/DefaultLut.ankitex"));

	m_fbDescr.m_colorAttachmentCount = 1;
	m_fbDescr.m_colorAttachments[0].m_loadOperation = AttachmentLoadOperation::DONT_CARE;
	m_fbDescr.bake();

	ANKI_CHECK(getResourceManager().loadResource("EngineAssets/BlueNoise_Rgba8_64x64.png", m_blueNoise));

	// Progs
	ANKI_CHECK(getResourceManager().loadResource("Shaders/FinalComposite.ankiprog", m_prog));

	ShaderProgramResourceVariantInitInfo variantInitInfo(m_prog);
	variantInitInfo.addMutation("BLUE_NOISE", 1);
	variantInitInfo.addMutation("BLOOM_ENABLED", 1);
	variantInitInfo.addConstant("LUT_SIZE", U32(LUT_SIZE));
	variantInitInfo.addConstant("FB_SIZE", m_r->getPostProcessResolution());
	variantInitInfo.addConstant("MOTION_BLUR_SAMPLES", config.getNumberU32("r_motionBlurSamples"));

	for(U32 dbg = 0; dbg < 2; ++dbg)
	{
		const ShaderProgramResourceVariant* variant;
		variantInitInfo.addMutation("DBG_ENABLED", dbg);
		m_prog->getOrCreateVariant(variantInitInfo, variant);
		m_grProgs[dbg] = variant->getProgram();
	}

	ANKI_CHECK(getResourceManager().loadResource("Shaders/VisualizeRenderTarget.ankiprog",
												 m_defaultVisualizeRenderTargetProg));
	const ShaderProgramResourceVariant* variant;
	m_defaultVisualizeRenderTargetProg->getOrCreateVariant(variant);
	m_defaultVisualizeRenderTargetGrProg = variant->getProgram();

	return Error::NONE;
}

Error FinalComposite::init(const ConfigSet& config)
{
	Error err = initInternal(config);
	if(err)
	{
		ANKI_R_LOGE("Failed to init PPS");
	}

	return err;
}

Error FinalComposite::loadColorGradingTextureImage(CString filename)
{
	m_lut.reset(nullptr);
	ANKI_CHECK(getResourceManager().loadResource(filename, m_lut));
	ANKI_ASSERT(m_lut->getWidth() == LUT_SIZE);
	ANKI_ASSERT(m_lut->getHeight() == LUT_SIZE);
	ANKI_ASSERT(m_lut->getDepth() == LUT_SIZE);

	return Error::NONE;
}

void FinalComposite::populateRenderGraph(RenderingContext& ctx)
{
	RenderGraphDescription& rgraph = ctx.m_renderGraphDescr;

	// Create the pass
	GraphicsRenderPassDescription& pass = rgraph.newGraphicsRenderPass("Final Composite");

	pass.setWork([this, &ctx](RenderPassWorkContext& rgraphCtx) {
		run(ctx, rgraphCtx);
	});
	pass.setFramebufferInfo(m_fbDescr, {ctx.m_outRenderTarget}, {});

	pass.newDependency({ctx.m_outRenderTarget, TextureUsageBit::FRAMEBUFFER_ATTACHMENT_WRITE});

	if(m_r->getDbg().getEnabled())
	{
		pass.newDependency({m_r->getDbg().getRt(), TextureUsageBit::SAMPLED_FRAGMENT});
	}

	pass.newDependency({m_r->getScale().getRt(), TextureUsageBit::SAMPLED_FRAGMENT});
	pass.newDependency({m_r->getBloom().getRt(), TextureUsageBit::SAMPLED_FRAGMENT});
	pass.newDependency({m_r->getMotionVectors().getMotionVectorsRt(), TextureUsageBit::SAMPLED_FRAGMENT});
	pass.newDependency({m_r->getGBuffer().getDepthRt(), TextureUsageBit::SAMPLED_FRAGMENT});

	RenderTargetHandle dbgRt;
	Bool dbgRtValid;
	ShaderProgramPtr debugProgram;
	m_r->getCurrentDebugRenderTarget(dbgRt, dbgRtValid, debugProgram);
	if(dbgRtValid)
	{
		pass.newDependency({dbgRt, TextureUsageBit::SAMPLED_FRAGMENT});
	}
}

void FinalComposite::run(RenderingContext& ctx, RenderPassWorkContext& rgraphCtx)
{
	CommandBufferPtr& cmdb = rgraphCtx.m_commandBuffer;
	const Bool dbgEnabled = m_r->getDbg().getEnabled();
	RenderTargetHandle dbgRt;
	Bool dbgRtValid;
	ShaderProgramPtr optionalDebugProgram;
	m_r->getCurrentDebugRenderTarget(dbgRt, dbgRtValid, optionalDebugProgram);

	// Bind program
	if(dbgRtValid && optionalDebugProgram.isCreated())
	{
		cmdb->bindShaderProgram(optionalDebugProgram);
	}
	else if(dbgRtValid)
	{
		cmdb->bindShaderProgram(m_defaultVisualizeRenderTargetGrProg);
	}
	else
	{
		cmdb->bindShaderProgram(m_grProgs[dbgEnabled]);
	}

	// Bind stuff
	if(!dbgRtValid)
	{
		cmdb->bindSampler(0, 0, m_r->getSamplers().m_nearestNearestClamp);
		cmdb->bindSampler(0, 1, m_r->getSamplers().m_trilinearClamp);
		cmdb->bindSampler(0, 2, m_r->getSamplers().m_trilinearRepeat);

		rgraphCtx.bindColorTexture(0, 3, m_r->getScale().getRt());

		rgraphCtx.bindColorTexture(0, 4, m_r->getBloom().getRt());
		cmdb->bindTexture(0, 5, m_lut->getTextureView());
		cmdb->bindTexture(0, 6, m_blueNoise->getTextureView());
		rgraphCtx.bindColorTexture(0, 7, m_r->getMotionVectors().getMotionVectorsRt());
		rgraphCtx.bindTexture(0, 8, m_r->getGBuffer().getDepthRt(),
							  TextureSubresourceInfo(DepthStencilAspectBit::DEPTH));

		if(dbgEnabled)
		{
			rgraphCtx.bindColorTexture(0, 9, m_r->getDbg().getRt());
		}

		UVec4 frameCountPad3;
		frameCountPad3.x() = m_r->getFrameCount() & MAX_U32;
		cmdb->setPushConstants(&frameCountPad3, sizeof(frameCountPad3));
	}
	else
	{
		rgraphCtx.bindColorTexture(0, 0, dbgRt);
		cmdb->bindSampler(0, 1, m_r->getSamplers().m_nearestNearestClamp);
	}

	cmdb->setViewport(0, 0, ctx.m_outRenderTargetWidth, ctx.m_outRenderTargetHeight);
	drawQuad(cmdb);

	// Draw UI
	m_r->getUiStage().draw(ctx.m_outRenderTargetWidth, ctx.m_outRenderTargetHeight, ctx, cmdb);
}

} // end namespace anki
