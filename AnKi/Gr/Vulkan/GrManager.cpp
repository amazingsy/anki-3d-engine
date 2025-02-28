// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr/GrManager.h>
#include <AnKi/Gr/Vulkan/GrManagerImpl.h>

#include <AnKi/Gr/Buffer.h>
#include <AnKi/Gr/Texture.h>
#include <AnKi/Gr/TextureView.h>
#include <AnKi/Gr/Sampler.h>
#include <AnKi/Gr/Shader.h>
#include <AnKi/Gr/ShaderProgram.h>
#include <AnKi/Gr/CommandBuffer.h>
#include <AnKi/Gr/Framebuffer.h>
#include <AnKi/Gr/OcclusionQuery.h>
#include <AnKi/Gr/TimestampQuery.h>
#include <AnKi/Gr/RenderGraph.h>
#include <AnKi/Gr/AccelerationStructure.h>

namespace anki {

GrManager::GrManager()
{
}

GrManager::~GrManager()
{
	// Destroy in reverse order
	m_cacheDir.destroy(m_alloc);
}

Error GrManager::newInstance(GrManagerInitInfo& init, GrManager*& gr)
{
	auto alloc = HeapAllocator<U8>(init.m_allocCallback, init.m_allocCallbackUserData, "Gr");

	GrManagerImpl* impl = alloc.newInstance<GrManagerImpl>();

	// Init
	impl->m_alloc = alloc;
	impl->m_cacheDir.create(alloc, init.m_cacheDirectory);
	Error err = impl->init(init);

	if(err)
	{
		alloc.deleteInstance(impl);
		gr = nullptr;
	}
	else
	{
		gr = impl;
	}

	return err;
}

void GrManager::deleteInstance(GrManager* gr)
{
	if(gr == nullptr)
	{
		return;
	}

	auto alloc = gr->m_alloc;
	gr->~GrManager();
	alloc.deallocate(gr, 1);
}

TexturePtr GrManager::acquireNextPresentableTexture()
{
	ANKI_VK_SELF(GrManagerImpl);
	return self.acquireNextPresentableTexture();
}

void GrManager::swapBuffers()
{
	ANKI_VK_SELF(GrManagerImpl);
	self.endFrame();
}

void GrManager::finish()
{
	ANKI_VK_SELF(GrManagerImpl);
	self.finish();
}

GrManagerStats GrManager::getStats() const
{
	ANKI_VK_SELF_CONST(GrManagerImpl);
	GrManagerStats out;

	self.getGpuMemoryManager().getAllocatedMemory(out.m_gpuMemory, out.m_cpuMemory);
	out.m_commandBufferCount = self.getCommandBufferFactory().getCreatedCommandBufferCount();

	return out;
}

BufferPtr GrManager::newBuffer(const BufferInitInfo& init)
{
	return BufferPtr(Buffer::newInstance(this, init));
}

TexturePtr GrManager::newTexture(const TextureInitInfo& init)
{
	return TexturePtr(Texture::newInstance(this, init));
}

TextureViewPtr GrManager::newTextureView(const TextureViewInitInfo& init)
{
	return TextureViewPtr(TextureView::newInstance(this, init));
}

SamplerPtr GrManager::newSampler(const SamplerInitInfo& init)
{
	return SamplerPtr(Sampler::newInstance(this, init));
}

ShaderPtr GrManager::newShader(const ShaderInitInfo& init)
{
	return ShaderPtr(Shader::newInstance(this, init));
}

ShaderProgramPtr GrManager::newShaderProgram(const ShaderProgramInitInfo& init)
{
	return ShaderProgramPtr(ShaderProgram::newInstance(this, init));
}

CommandBufferPtr GrManager::newCommandBuffer(const CommandBufferInitInfo& init)
{
	return CommandBufferPtr(CommandBuffer::newInstance(this, init));
}

FramebufferPtr GrManager::newFramebuffer(const FramebufferInitInfo& init)
{
	return FramebufferPtr(Framebuffer::newInstance(this, init));
}

OcclusionQueryPtr GrManager::newOcclusionQuery()
{
	return OcclusionQueryPtr(OcclusionQuery::newInstance(this));
}

TimestampQueryPtr GrManager::newTimestampQuery()
{
	return TimestampQueryPtr(TimestampQuery::newInstance(this));
}

RenderGraphPtr GrManager::newRenderGraph()
{
	return RenderGraphPtr(RenderGraph::newInstance(this));
}

AccelerationStructurePtr GrManager::newAccelerationStructure(const AccelerationStructureInitInfo& init)
{
	return AccelerationStructurePtr(AccelerationStructure::newInstance(this, init));
}

} // end namespace anki
