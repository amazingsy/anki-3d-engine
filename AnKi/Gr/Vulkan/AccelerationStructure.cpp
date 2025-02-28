// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr/AccelerationStructure.h>
#include <AnKi/Gr/Vulkan/AccelerationStructureImpl.h>
#include <AnKi/Gr/GrManager.h>

namespace anki {

AccelerationStructure* AccelerationStructure::newInstance(GrManager* manager, const AccelerationStructureInitInfo& init)
{
	AccelerationStructureImpl* impl =
		manager->getAllocator().newInstance<AccelerationStructureImpl>(manager, init.getName());
	const Error err = impl->init(init);
	if(err)
	{
		manager->getAllocator().deleteInstance(impl);
		impl = nullptr;
	}
	return impl;
}

} // end namespace anki
