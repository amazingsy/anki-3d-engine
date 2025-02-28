// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr/Fence.h>
#include <AnKi/Gr/Vulkan/FenceImpl.h>
#include <AnKi/Gr/GrManager.h>

namespace anki {

Fence* Fence::newInstance(GrManager* manager)
{
	return manager->getAllocator().newInstance<FenceImpl>(manager, "N/A");
}

Bool Fence::clientWait(Second seconds)
{
	return static_cast<FenceImpl*>(this)->m_semaphore->clientWait(seconds);
}

} // end namespace anki
