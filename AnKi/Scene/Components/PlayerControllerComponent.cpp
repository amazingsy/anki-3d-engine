// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Scene/Components/PlayerControllerComponent.h>
#include <AnKi/Scene/SceneNode.h>
#include <AnKi/Scene/SceneGraph.h>
#include <AnKi/Physics/PhysicsWorld.h>

namespace anki {

ANKI_SCENE_COMPONENT_STATICS(PlayerControllerComponent)

PlayerControllerComponent::PlayerControllerComponent(SceneNode* node)
	: SceneComponent(node, getStaticClassId())
{
	PhysicsPlayerControllerInitInfo init;
	init.m_position = Vec3(0.0f);
	m_player = node->getSceneGraph().getPhysicsWorld().newInstance<PhysicsPlayerController>(init);
	m_player->setUserData(this);
}

} // end namespace anki
