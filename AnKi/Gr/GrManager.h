// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Gr/Common.h>
#include <AnKi/Gr/GrObject.h>
#include <AnKi/Util/String.h>

namespace anki {

// Forward
class ConfigSet;
class NativeWindow;

/// @addtogroup graphics
/// @{

/// Manager initializer.
class GrManagerInitInfo
{
public:
	AllocAlignedCallback m_allocCallback = nullptr;
	void* m_allocCallbackUserData = nullptr;

	CString m_cacheDirectory;

	const ConfigSet* m_config = nullptr;
	NativeWindow* m_window = nullptr;
};

/// Graphics statistics.
class GrManagerStats
{
public:
	PtrSize m_cpuMemory = 0;
	PtrSize m_gpuMemory = 0;
	U32 m_commandBufferCount = 0;
};

/// The graphics manager, owner of all graphics objects.
class GrManager
{
public:
	/// Create.
	static ANKI_USE_RESULT Error newInstance(GrManagerInitInfo& init, GrManager*& gr);

	/// Destroy.
	static void deleteInstance(GrManager* gr);

	const GpuDeviceCapabilities& getDeviceCapabilities() const
	{
		return m_capabilities;
	}

	const BindlessLimits& getBindlessLimits() const
	{
		return m_bindlessLimits;
	}

	/// Get next presentable image. The returned Texture is valid until the following swapBuffers. After that it might
	/// dissapear even if you hold the reference.
	TexturePtr acquireNextPresentableTexture();

	/// Swap buffers
	void swapBuffers();

	/// Wait for all work to finish.
	void finish();

	/// @name Object creation methods. They are thread-safe.
	/// @{
	ANKI_USE_RESULT BufferPtr newBuffer(const BufferInitInfo& init);
	ANKI_USE_RESULT TexturePtr newTexture(const TextureInitInfo& init);
	ANKI_USE_RESULT TextureViewPtr newTextureView(const TextureViewInitInfo& init);
	ANKI_USE_RESULT SamplerPtr newSampler(const SamplerInitInfo& init);
	ANKI_USE_RESULT ShaderPtr newShader(const ShaderInitInfo& init);
	ANKI_USE_RESULT ShaderProgramPtr newShaderProgram(const ShaderProgramInitInfo& init);
	ANKI_USE_RESULT CommandBufferPtr newCommandBuffer(const CommandBufferInitInfo& init);
	ANKI_USE_RESULT FramebufferPtr newFramebuffer(const FramebufferInitInfo& init);
	ANKI_USE_RESULT OcclusionQueryPtr newOcclusionQuery();
	ANKI_USE_RESULT TimestampQueryPtr newTimestampQuery();
	ANKI_USE_RESULT RenderGraphPtr newRenderGraph();
	ANKI_USE_RESULT AccelerationStructurePtr newAccelerationStructure(const AccelerationStructureInitInfo& init);
	/// @}

	GrManagerStats getStats() const;

	ANKI_INTERNAL GrAllocator<U8>& getAllocator()
	{
		return m_alloc;
	}

	ANKI_INTERNAL GrAllocator<U8> getAllocator() const
	{
		return m_alloc;
	}

	ANKI_INTERNAL CString getCacheDirectory() const
	{
		return m_cacheDir.toCString();
	}

	ANKI_INTERNAL U64 getNewUuid()
	{
		return m_uuidIndex.fetchAdd(1);
	}

protected:
	GrAllocator<U8> m_alloc; ///< Keep it first to get deleted last
	String m_cacheDir;
	Atomic<U64> m_uuidIndex = {1};
	GpuDeviceCapabilities m_capabilities;
	BindlessLimits m_bindlessLimits;

	GrManager();

	virtual ~GrManager();
};
/// @}

} // end namespace anki
