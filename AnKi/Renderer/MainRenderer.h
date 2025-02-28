// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/Common.h>
#include <AnKi/Resource/Forward.h>
#include <AnKi/Renderer/Renderer.h>

namespace anki {

// Forward
class ResourceManager;
class ConfigSet;
class StagingGpuMemoryPool;
class UiManager;

/// @addtogroup renderer
/// @{

/// MainRenderer statistics.
class MainRendererStats
{
public:
	Second m_renderingCpuTime ANKI_DEBUG_CODE(= -1.0);
	Second m_renderingGpuTime ANKI_DEBUG_CODE(= -1.0);
	Second m_renderingGpuSubmitTimestamp ANKI_DEBUG_CODE(= -1.0);
};

/// Main onscreen renderer
class MainRenderer
{
public:
	MainRenderer();

	~MainRenderer();

	ANKI_USE_RESULT Error init(ThreadHive* hive, ResourceManager* resources, GrManager* gl,
							   StagingGpuMemoryPool* stagingMem, UiManager* ui, AllocAlignedCallback allocCb,
							   void* allocCbUserData, const ConfigSet& config, Timestamp* globTimestamp);

	ANKI_USE_RESULT Error render(RenderQueue& rqueue, TexturePtr presentTex);

	Dbg& getDbg();

	F32 getAspectRatio() const;

	const Renderer& getOffscreenRenderer() const
	{
		return *m_r;
	}

	Renderer& getOffscreenRenderer()
	{
		return *m_r;
	}

	void setStatsEnabled(Bool enabled)
	{
		m_statsEnabled = enabled;
	}

	const MainRendererStats& getStats() const
	{
		return m_stats;
	}

private:
	HeapAllocator<U8> m_alloc;
	StackAllocator<U8> m_frameAlloc;

	UniquePtr<Renderer> m_r;
	Bool m_rDrawToDefaultFb = false;

	ShaderProgramResourcePtr m_blitProg;
	ShaderProgramPtr m_blitGrProg;

	UVec2 m_swapchainResolution = UVec2(0u);
	F32 m_renderScaling = 1.0f;

	RenderGraphPtr m_rgraph;
	RenderTargetDescription m_tmpRtDesc;
	FramebufferDescription m_fbDescr;

	MainRendererStats m_stats;
	Bool m_statsEnabled = false;

	class
	{
	public:
		const RenderingContext* m_ctx = nullptr;
		Atomic<U32> m_secondaryTaskId = {0};
	} m_runCtx;
};
/// @}

} // end namespace anki
