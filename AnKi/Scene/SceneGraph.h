// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Scene/Common.h>
#include <AnKi/Scene/SceneNode.h>
#include <AnKi/Scene/DebugDrawer.h>
#include <AnKi/Math.h>
#include <AnKi/Util/HashMap.h>
#include <AnKi/Core/App.h>
#include <AnKi/Scene/Events/EventManager.h>
#include <AnKi/Resource/Common.h>

namespace anki {

// Forward
class ResourceManager;
class CameraNode;
class Input;
class ConfigSet;
class PerspectiveCameraNode;
class Octree;
class UiManager;

/// @addtogroup scene
/// @{

/// SceneGraph statistics.
class SceneGraphStats
{
public:
	Second m_updateTime ANKI_DEBUG_CODE(= 0.0);
	Second m_visibilityTestsTime ANKI_DEBUG_CODE(= 0.0);
	Second m_physicsUpdate ANKI_DEBUG_CODE(= 0.0);
};

/// SceneGraph limits.
class SceneGraphConfig
{
public:
	F32 m_earlyZDistance = -1.0f; ///< Objects with distance lower than that will be used in early Z.
	F32 m_reflectionProbeEffectiveDistance = -1.0f; ///< How far reflection probes can look.
	F32 m_reflectionProbeShadowEffectiveDistance = -1.0f; ///< How far to render shadows for reflection probes.
	Bool m_rayTracedShadows = false;
	F32 m_rayTracingExtendedFrustumDistance = 100.0f; ///< The frustum distance from the eye to every direction.
	Array<F32, MAX_LOD_COUNT - 1> m_maxLodDistances = {};
};

/// The scene graph that  all the scene entities
class SceneGraph
{
	friend class SceneNode;
	friend class UpdateSceneNodesTask;

public:
	SceneGraph();

	~SceneGraph();

	ANKI_USE_RESULT Error init(AllocAlignedCallback allocCb, void* allocCbData, ThreadHive* threadHive,
							   ResourceManager* resources, Input* input, ScriptManager* scriptManager,
							   UiManager* uiManager, const Timestamp* globalTimestamp, const ConfigSet& config);

	Timestamp getGlobalTimestamp() const
	{
		return m_timestamp;
	}

	/// @note Return a copy
	SceneAllocator<U8> getAllocator() const
	{
		return m_alloc;
	}

	/// @note Return a copy
	SceneFrameAllocator<U8> getFrameAllocator() const
	{
		return m_frameAlloc;
	}

	SceneNode& getActiveCameraNode()
	{
		ANKI_ASSERT(m_mainCam != nullptr);
		return *m_mainCam;
	}
	const SceneNode& getActiveCameraNode() const
	{
		return *m_mainCam;
	}
	void setActiveCameraNode(SceneNode* cam)
	{
		m_mainCam = cam;
		m_activeCameraChangeTimestamp = getGlobalTimestamp();
	}
	Timestamp getActiveCameraNodeChangeTimestamp() const
	{
		return m_activeCameraChangeTimestamp;
	}

	U32 getSceneNodesCount() const
	{
		return m_nodesCount;
	}

	EventManager& getEventManager()
	{
		return m_events;
	}
	const EventManager& getEventManager() const
	{
		return m_events;
	}

	ThreadHive& getThreadHive()
	{
		return *m_threadHive;
	}

	ANKI_USE_RESULT Error update(Second prevUpdateTime, Second crntTime);

	void doVisibilityTests(RenderQueue& rqueue);

	SceneNode& findSceneNode(const CString& name);
	SceneNode* tryFindSceneNode(const CString& name);

	/// Iterate the scene nodes using a lambda
	template<typename Func>
	ANKI_USE_RESULT Error iterateSceneNodes(Func func)
	{
		for(SceneNode& psn : m_nodes)
		{
			Error err = func(psn);
			if(err)
			{
				return err;
			}
		}

		return Error::NONE;
	}

	/// Iterate a range of scene nodes using a lambda
	template<typename Func>
	ANKI_USE_RESULT Error iterateSceneNodes(PtrSize begin, PtrSize end, Func func);

	/// Create a new SceneNode
	template<typename Node, typename... Args>
	ANKI_USE_RESULT Error newSceneNode(const CString& name, Node*& node, Args&&... args);

	/// Delete a scene node. It actualy marks it for deletion
	void deleteSceneNode(SceneNode* node)
	{
		node->setMarkedForDeletion();
	}

	void increaseObjectsMarkedForDeletion()
	{
		m_objectsMarkedForDeletionCount.fetchAdd(1);
	}

	const SceneGraphStats& getStats() const
	{
		return m_stats;
	}

	const SceneGraphConfig& getConfig() const
	{
		return m_config;
	}

	const Vec3& getSceneMin() const
	{
		return m_sceneMin;
	}

	const Vec3& getSceneMax() const
	{
		return m_sceneMax;
	}

	ResourceManager& getResourceManager()
	{
		return *m_resources;
	}

	GrManager& getGrManager()
	{
		return *m_gr;
	}

	PhysicsWorld& getPhysicsWorld()
	{
		return *m_physics;
	}

	ScriptManager& getScriptManager()
	{
		ANKI_ASSERT(m_scriptManager);
		return *m_scriptManager;
	}

	const PhysicsWorld& getPhysicsWorld() const
	{
		return *m_physics;
	}

	const Input& getInput() const
	{
		ANKI_ASSERT(m_input);
		return *m_input;
	}

	UiManager& getUiManager()
	{
		return *m_uiManager;
	}

	U64 getNewUuid()
	{
		return m_nodesUuid.fetchAdd(1);
	}

	Octree& getOctree()
	{
		ANKI_ASSERT(m_octree);
		return *m_octree;
	}

	const DebugDrawer2& getDebugDrawer() const
	{
		return m_debugDrawer;
	}

private:
	class UpdateSceneNodesCtx;

	const Timestamp* m_globalTimestamp = nullptr;
	Timestamp m_timestamp = 0; ///< Cached timestamp

	// Sub-systems
	ThreadHive* m_threadHive = nullptr;
	ResourceManager* m_resources = nullptr;
	GrManager* m_gr = nullptr;
	PhysicsWorld* m_physics = nullptr;
	Input* m_input = nullptr;
	ScriptManager* m_scriptManager = nullptr;
	UiManager* m_uiManager = nullptr;

	SceneAllocator<U8> m_alloc;
	SceneFrameAllocator<U8> m_frameAlloc;

	IntrusiveList<SceneNode> m_nodes;
	U32 m_nodesCount = 0;
	HashMap<CString, SceneNode*> m_nodesDict;

	SceneNode* m_mainCam = nullptr;
	Timestamp m_activeCameraChangeTimestamp = 0;
	PerspectiveCameraNode* m_defaultMainCam = nullptr;

	EventManager m_events;

	Octree* m_octree = nullptr;

	Vec3 m_sceneMin = Vec3(-1000.0f, -200.0f, -1000.0f);
	Vec3 m_sceneMax = Vec3(1000.0f, 200.0f, 1000.0f);

	Atomic<U32> m_objectsMarkedForDeletionCount = {0};

	Atomic<U64> m_nodesUuid = {1};

	SceneGraphConfig m_config;
	SceneGraphStats m_stats;

	DebugDrawer2 m_debugDrawer;

	/// Put a node in the appropriate containers
	ANKI_USE_RESULT Error registerNode(SceneNode* node);
	void unregisterNode(SceneNode* node);

	/// Delete the nodes that are marked for deletion
	void deleteNodesMarkedForDeletion();

	ANKI_USE_RESULT Error updateNodes(UpdateSceneNodesCtx& ctx) const;
	ANKI_USE_RESULT static Error updateNode(Second prevTime, Second crntTime, SceneNode& node);

	/// Do visibility tests.
	static void doVisibilityTests(SceneNode& frustumable, SceneGraph& scene, RenderQueue& rqueue);
};

template<typename Node, typename... Args>
inline Error SceneGraph::newSceneNode(const CString& name, Node*& node, Args&&... args)
{
	Error err = Error::NONE;
	SceneAllocator<Node> al = m_alloc;

	node = al.template newInstance<Node>(this, name);
	if(node)
	{
		err = node->init(std::forward<Args>(args)...);
	}
	else
	{
		err = Error::OUT_OF_MEMORY;
	}

	if(!err)
	{
		err = registerNode(node);
	}

	if(err)
	{
		ANKI_SCENE_LOGE("Failed to create scene node: %s", (name.isEmpty()) ? "unnamed" : &name[0]);

		if(node)
		{
			al.deleteInstance(node);
			node = nullptr;
		}
	}

	return err;
}

template<typename Func>
Error SceneGraph::iterateSceneNodes(PtrSize begin, PtrSize end, Func func)
{
	ANKI_ASSERT(begin < m_nodesCount && end <= m_nodesCount);
	auto it = m_nodes.getBegin() + begin;

	PtrSize count = end - begin;
	Error err = Error::NONE;
	while(count-- != 0 && !err)
	{
		ANKI_ASSERT(it != m_nodes.getEnd());
		err = func(*it);

		++it;
	}

	return Error::NONE;
}
/// @}

} // end namespace anki
