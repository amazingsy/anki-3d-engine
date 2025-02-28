// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Scene/SceneNode.h>

namespace anki {

/// @addtogroup scene
/// @{

/// Node that gets affected by physics.
class BodyNode : public SceneNode
{
public:
	BodyNode(SceneGraph* scene, CString name);

	~BodyNode();

private:
	class FeedbackComponent;
};
/// @}

} // end namespace anki
