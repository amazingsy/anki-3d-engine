// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Scene/Components/SceneComponent.h>
#include <AnKi/Renderer/RenderQueue.h>

namespace anki {

/// @addtogroup scene
/// @{

/// Holds some info for GPU compute jobs.
class GenericGpuComputeJobComponent : public SceneComponent
{
	ANKI_SCENE_COMPONENT(GenericGpuComputeJobComponent)

public:
	GenericGpuComputeJobComponent(SceneNode* node)
		: SceneComponent(node, getStaticClassId())
	{
	}

	~GenericGpuComputeJobComponent()
	{
	}

	void setCallback(GenericGpuComputeJobQueueElementCallback callback, const void* userData)
	{
		ANKI_ASSERT(callback && userData);
		m_callback = callback;
		m_userData = userData;
	}

	void setupGenericGpuComputeJobQueueElement(GenericGpuComputeJobQueueElement& el)
	{
		ANKI_ASSERT(m_callback && m_userData);
		el.m_callback = m_callback;
		el.m_userData = m_userData;
	}

private:
	GenericGpuComputeJobQueueElementCallback m_callback = nullptr;
	const void* m_userData = nullptr;
};
/// @}

} // end namespace anki
