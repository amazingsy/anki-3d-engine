// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Scene/SceneNode.h>
#include <AnKi/Collision/Aabb.h>

namespace anki {

/// @addtogroup scene
/// @{

/// Fog density node.
class FogDensityNode : public SceneNode
{
public:
	FogDensityNode(SceneGraph* scene, CString name);

	~FogDensityNode();

private:
	class MoveFeedbackComponent;
	class DensityShapeFeedbackComponent;

	void onMoveUpdated(const MoveComponent& movec);
	void onDensityShapeUpdated(const FogDensityComponent& fogc);
};
/// @}

} // end namespace anki
