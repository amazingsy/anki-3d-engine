// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Gr/GrObject.h>
#include <AnKi/Gr/GrManager.h>

namespace anki {

GrObject::GrObject(GrManager* manager, GrObjectType type, CString name)
	: m_manager(manager)
	, m_uuid(m_manager->getNewUuid())
	, m_refcount(0)
	, m_type(type)
{
	if(name.getLength() == 0)
	{
		name = "N/A";
	}

	m_name = static_cast<Char*>(manager->getAllocator().getMemoryPool().allocate(name.getLength() + 1, alignof(Char)));
	memcpy(m_name, &name[0], name.getLength() + 1);
}

GrObject::~GrObject()
{
	if(m_name)
	{
		m_manager->getAllocator().getMemoryPool().free(m_name);
	}
}

GrAllocator<U8> GrObject::getAllocator() const
{
	return m_manager->getAllocator();
}

} // end namespace anki
