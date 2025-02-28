// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Physics/PhysicsJoint.h>
#include <AnKi/Physics/PhysicsBody.h>
#include <AnKi/Physics/PhysicsWorld.h>

namespace anki {

PhysicsJoint::PhysicsJoint(PhysicsWorld* world, JointType type)
	: PhysicsObject(CLASS_TYPE, world)
	, m_type(type)
{
}

void PhysicsJoint::registerToWorld()
{
	getWorld().getBtWorld().addConstraint(getJoint());
}

void PhysicsJoint::unregisterFromWorld()
{
	getWorld().getBtWorld().removeConstraint(getJoint());
}

PhysicsPoint2PointJoint::PhysicsPoint2PointJoint(PhysicsWorld* world, PhysicsBodyPtr bodyA, const Vec3& relPos)
	: PhysicsJoint(world, JointType::P2P)
{
	m_bodyA = bodyA;
	m_p2p.init(*m_bodyA->getBtBody(), toBt(relPos));
	getJoint()->setUserConstraintPtr(static_cast<PhysicsObject*>(this));
}

PhysicsPoint2PointJoint::PhysicsPoint2PointJoint(PhysicsWorld* world, PhysicsBodyPtr bodyA, const Vec3& relPosA,
												 PhysicsBodyPtr bodyB, const Vec3& relPosB)
	: PhysicsJoint(world, JointType::P2P)
{
	ANKI_ASSERT(bodyA != bodyB);
	m_bodyA = bodyA;
	m_bodyB = bodyB;

	m_p2p.init(*m_bodyA->getBtBody(), *m_bodyB->getBtBody(), toBt(relPosA), toBt(relPosB));
	getJoint()->setUserConstraintPtr(static_cast<PhysicsObject*>(this));
}

PhysicsPoint2PointJoint::~PhysicsPoint2PointJoint()
{
	m_p2p.destroy();
}

PhysicsHingeJoint::PhysicsHingeJoint(PhysicsWorld* world, PhysicsBodyPtr bodyA, const Vec3& relPos, const Vec3& axis)
	: PhysicsJoint(world, JointType::HINGE)
{
	m_bodyA = bodyA;
	m_hinge.init(*m_bodyA->getBtBody(), toBt(relPos), toBt(axis));
	getJoint()->setUserConstraintPtr(static_cast<PhysicsObject*>(this));
}

PhysicsHingeJoint::~PhysicsHingeJoint()
{
	m_hinge.destroy();
}

} // end namespace anki
