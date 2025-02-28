// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Scene/TriggerNode.h>
#include <AnKi/Scene/SceneGraph.h>
#include <AnKi/Scene/Components/MoveComponent.h>
#include <AnKi/Scene/Components/TriggerComponent.h>
#include <AnKi/Physics/PhysicsWorld.h>
#include <AnKi/Physics/PhysicsCollisionShape.h>
#include <AnKi/Physics/PhysicsTrigger.h>

namespace anki {

class TriggerNode::MoveFeedbackComponent : public SceneComponent
{
	ANKI_SCENE_COMPONENT(TriggerNode::MoveFeedbackComponent)

public:
	MoveFeedbackComponent(SceneNode* node)
		: SceneComponent(node, getStaticClassId(), true)
	{
	}

	ANKI_USE_RESULT Error update(SceneNode& node, Second prevTime, Second crntTime, Bool& updated) final
	{
		updated = false;

		const MoveComponent& move = node.getFirstComponentOfType<MoveComponent>();
		if(move.getTimestamp() == node.getGlobalTimestamp())
		{
			node.getFirstComponentOfType<TriggerComponent>().setWorldTransform(move.getWorldTransform());
		}

		return Error::NONE;
	}
};

ANKI_SCENE_COMPONENT_STATICS(TriggerNode::MoveFeedbackComponent)

TriggerNode::TriggerNode(SceneGraph* scene, CString name)
	: SceneNode(scene, name)
{
	newComponent<MoveComponent>();
	newComponent<MoveFeedbackComponent>();
	newComponent<TriggerComponent>();
}

TriggerNode::~TriggerNode()
{
}

} // end namespace anki
