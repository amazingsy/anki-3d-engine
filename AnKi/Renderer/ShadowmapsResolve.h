// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/RendererObject.h>
#include <AnKi/Resource/ImageResource.h>
#include <AnKi/Gr.h>

namespace anki {

/// @addtogroup renderer
/// @{

/// Resolves shadowmaps into a single texture.
class ShadowmapsResolve : public RendererObject
{
public:
	ShadowmapsResolve(Renderer* r)
		: RendererObject(r)
	{
		registerDebugRenderTarget("SM_resolve");
	}

	~ShadowmapsResolve();

	ANKI_USE_RESULT Error init(const ConfigSet& cfg);

	void populateRenderGraph(RenderingContext& ctx);

	void getDebugRenderTarget(CString rtName, RenderTargetHandle& handle,
							  ShaderProgramPtr& optionalShaderProgram) const override
	{
		ANKI_ASSERT(rtName == "SM_resolve");
		handle = m_runCtx.m_rt;
	}

	RenderTargetHandle getRt() const
	{
		return m_runCtx.m_rt;
	}

public:
	ShaderProgramResourcePtr m_prog;
	ShaderProgramPtr m_grProg;
	RenderTargetDescription m_rtDescr;

	class
	{
	public:
		RenderTargetHandle m_rt;
	} m_runCtx;

	ANKI_USE_RESULT Error initInternal(const ConfigSet& cfg);

	void run(const RenderingContext& ctx, RenderPassWorkContext& rgraphCtx);
};
/// @}

} // namespace anki
