// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Core/Common.h>
#include <AnKi/Util/Allocator.h>
#include <AnKi/Util/String.h>
#include <AnKi/Util/Ptr.h>
#include <AnKi/Ui/UiImmediateModeBuilder.h>

namespace anki {

// Forward
class CoreTracer;
class ConfigSet;
class ThreadHive;
class NativeWindow;
class Input;
class GrManager;
class MainRenderer;
class PhysicsWorld;
class SceneGraph;
class ScriptManager;
class ResourceManager;
class ResourceFilesystem;
class StagingGpuMemoryPool;
class VertexGpuMemoryPool;
class UiManager;
class UiQueueElement;
class RenderQueue;
class MaliHwCounters;

/// The core class of the engine.
class App
{
public:
	App();
	virtual ~App();

	ANKI_USE_RESULT Error init(const ConfigSet& config, AllocAlignedCallback allocCb, void* allocCbUserData);

	Second getTimerTick() const
	{
		return m_timerTick;
	}

	void setTimerTick(const F32 x)
	{
		m_timerTick = x;
	}

	const String& getSettingsDirectory() const
	{
		return m_settingsDir;
	}

	const String& getCacheDirectory() const
	{
		return m_cacheDir;
	}

	AllocAlignedCallback getAllocationCallback() const
	{
		return m_allocCb;
	}

	void* getAllocationCallbackData() const
	{
		return m_allocCbData;
	}

	ThreadHive& getThreadHive()
	{
		return *m_threadHive;
	}

	HeapAllocator<U8>& getAllocator()
	{
		return m_heapAlloc;
	}

	Timestamp getGlobalTimestamp() const
	{
		return m_globalTimestamp;
	}

	/// Run the main loop.
	ANKI_USE_RESULT Error mainLoop();

	/// The user code to run along with the other main loop code.
	virtual ANKI_USE_RESULT Error userMainLoop(Bool& quit, Second elapsedTime)
	{
		// Do nothing
		return Error::NONE;
	}

	Input& getInput()
	{
		return *m_input;
	}

	SceneGraph& getSceneGraph()
	{
		return *m_scene;
	}

	MainRenderer& getMainRenderer()
	{
		return *m_renderer;
	}

	ResourceManager& getResourceManager()
	{
		return *m_resources;
	}

	ScriptManager& getScriptManager()
	{
		return *m_script;
	}

	PhysicsWorld& getPhysicsWorld()
	{
		return *m_physics;
	}

	NativeWindow& getWindow()
	{
		return *m_window;
	}

	HeapAllocator<U8> getAllocator() const
	{
		return m_heapAlloc;
	}

	void setDisplayStats(Bool enable)
	{
		m_displayStats = enable;
	}

	Bool getDisplayStats() const
	{
		return m_displayStats;
	}

	void setDisplayDeveloperConsole(Bool display)
	{
		m_consoleEnabled = display;
	}

	Bool getDisplayDeveloperConsole() const
	{
		return m_consoleEnabled;
	}

private:
	// Allocation
	AllocAlignedCallback m_allocCb;
	void* m_allocCbData;
	HeapAllocator<U8> m_heapAlloc;

	// Sybsystems
#if ANKI_ENABLE_TRACE
	CoreTracer* m_coreTracer = nullptr;
#endif
	NativeWindow* m_window = nullptr;
	Input* m_input = nullptr;
	GrManager* m_gr = nullptr;
	MaliHwCounters* m_maliHwCounters = nullptr;
	VertexGpuMemoryPool* m_vertexMem = nullptr;
	StagingGpuMemoryPool* m_stagingMem = nullptr;
	PhysicsWorld* m_physics = nullptr;
	ResourceFilesystem* m_resourceFs = nullptr;
	ResourceManager* m_resources = nullptr;
	UiManager* m_ui = nullptr;
	MainRenderer* m_renderer = nullptr;
	SceneGraph* m_scene = nullptr;
	ScriptManager* m_script = nullptr;

	// Misc
	UiImmediateModeBuilderPtr m_statsUi;
	Bool m_displayStats = false;
	UiImmediateModeBuilderPtr m_console;
	Bool m_consoleEnabled = false;
	Timestamp m_globalTimestamp = 1;
	ThreadHive* m_threadHive = nullptr;
	String m_settingsDir; ///< The path that holds the configuration
	String m_cacheDir; ///< This is used as a cache
	Second m_timerTick;
	U64 m_resourceCompletedAsyncTaskCount = 0;

	class MemStats
	{
	public:
		Atomic<PtrSize> m_allocatedMem = {0};
		Atomic<U64> m_allocCount = {0};
		Atomic<U64> m_freeCount = {0};

		void* m_originalUserData = nullptr;
		AllocAlignedCallback m_originalAllocCallback = nullptr;

		static void* allocCallback(void* userData, void* ptr, PtrSize size, PtrSize alignment);
	} m_memStats;

	void initMemoryCallbacks(AllocAlignedCallback allocCb, void* allocCbUserData);

	ANKI_USE_RESULT Error initInternal(const ConfigSet& config, AllocAlignedCallback allocCb, void* allocCbUserData);

	ANKI_USE_RESULT Error initDirs(const ConfigSet& cfg);
	void cleanup();

	/// Inject a new UI element in the render queue for displaying various stuff.
	void injectUiElements(DynamicArrayAuto<UiQueueElement>& elements, RenderQueue& rqueue);

	void setSignalHandlers();
};

} // end namespace anki
