// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include <AnKi/Resource/ResourceManager.h>
#include <AnKi/Resource/AsyncLoader.h>
#include <AnKi/Resource/ShaderProgramResourceSystem.h>
#include <AnKi/Resource/AnimationResource.h>
#include <AnKi/Util/Logger.h>
#include <AnKi/Core/ConfigSet.h>

#include <AnKi/Resource/MaterialResource.h>
#include <AnKi/Resource/MeshResource.h>
#include <AnKi/Resource/CpuMeshResource.h>
#include <AnKi/Resource/ModelResource.h>
#include <AnKi/Resource/ScriptResource.h>
#include <AnKi/Resource/DummyResource.h>
#include <AnKi/Resource/ParticleEmitterResource.h>
#include <AnKi/Resource/ImageResource.h>
#include <AnKi/Resource/GenericResource.h>
#include <AnKi/Resource/ImageAtlasResource.h>
#include <AnKi/Resource/ShaderProgramResource.h>
#include <AnKi/Resource/SkeletonResource.h>

namespace anki {

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
	m_cacheDir.destroy(m_alloc);
	m_alloc.deleteInstance(m_asyncLoader);
	m_alloc.deleteInstance(m_shaderProgramSystem);
	m_alloc.deleteInstance(m_transferGpuAlloc);
}

Error ResourceManager::init(ResourceManagerInitInfo& init)
{
	m_gr = init.m_gr;
	m_physics = init.m_physics;
	m_fs = init.m_resourceFs;
	m_vertexMem = init.m_vertexMemory;
	m_alloc = ResourceAllocator<U8>(init.m_allocCallback, init.m_allocCallbackData);

	m_tmpAlloc = TempResourceAllocator<U8>(init.m_allocCallback, init.m_allocCallbackData, 10_MB);

	m_cacheDir.create(m_alloc, init.m_cacheDir);

	// Init some constants
	m_maxImageSize = init.m_config->getNumberU32("rsrc_maxImageSize");
	m_dumpShaderSource = init.m_config->getBool("rsrc_dumpShaderSources");

	// Init type resource managers
#define ANKI_INSTANTIATE_RESOURCE(rsrc_, ptr_) TypeResourceManager<rsrc_>::init(m_alloc);
#define ANKI_INSTANSIATE_RESOURCE_DELIMITER()
#include <AnKi/Resource/InstantiationMacros.h>
#undef ANKI_INSTANTIATE_RESOURCE
#undef ANKI_INSTANSIATE_RESOURCE_DELIMITER

	// Init the thread
	m_asyncLoader = m_alloc.newInstance<AsyncLoader>();
	m_asyncLoader->init(m_alloc);

	m_transferGpuAlloc = m_alloc.newInstance<TransferGpuAllocator>();
	ANKI_CHECK(m_transferGpuAlloc->init(init.m_config->getNumberU32("rsrc_transferScratchMemorySize"), m_gr, m_alloc));

	// Init the programs
	m_shaderProgramSystem = m_alloc.newInstance<ShaderProgramResourceSystem>(m_cacheDir, m_gr, m_fs, m_alloc);
	ANKI_CHECK(m_shaderProgramSystem->init());

	return Error::NONE;
}

U64 ResourceManager::getAsyncTaskCompletedCount() const
{
	return m_asyncLoader->getCompletedTaskCount();
}

template<typename T>
Error ResourceManager::loadResource(const CString& filename, ResourcePtr<T>& out, Bool async)
{
	ANKI_ASSERT(!out.isCreated() && "Already loaded");

	Error err = Error::NONE;
	++m_loadRequestCount;

	T* const other = findLoadedResource<T>(filename);

	if(other)
	{
		// Found
		out.reset(other);
	}
	else
	{
		// Allocate ptr
		T* ptr = m_alloc.newInstance<T>(this);
		ANKI_ASSERT(ptr->getRefcount().load() == 0);

		// Increment the refcount in that case where async jobs increment it and decrement it in the scope of a load()
		ptr->getRefcount().fetchAdd(1);

		// Populate the ptr. Use a block to cleanup temp_pool allocations
		auto& pool = m_tmpAlloc.getMemoryPool();

		{
			U allocsCountBefore = pool.getAllocationCount();
			(void)allocsCountBefore;

			err = ptr->load(filename, async);
			if(err)
			{
				ANKI_RESOURCE_LOGE("Failed to load resource: %s", &filename[0]);
				m_alloc.deleteInstance(ptr);
				return err;
			}

			ANKI_ASSERT(pool.getAllocationCount() == allocsCountBefore && "Forgot to deallocate");
		}

		ptr->setFilename(filename);
		ptr->setUuid(++m_uuid);

		// Reset the memory pool if no-one is using it.
		// NOTE: Check because resources load other resources
		if(pool.getAllocationCount() == 0)
		{
			pool.reset();
		}

		// Register resource
		registerResource(ptr);
		out.reset(ptr);

		// Decrement because of the increment happened a few lines above
		ptr->getRefcount().fetchSub(1);
	}

	return err;
}

// Instansiate the ResourceManager::loadResource()
#define ANKI_INSTANTIATE_RESOURCE(rsrc_, ptr_) \
	template Error ResourceManager::loadResource<rsrc_>(const CString& filename, ResourcePtr<rsrc_>& out, Bool async);
#define ANKI_INSTANSIATE_RESOURCE_DELIMITER()
#include <AnKi/Resource/InstantiationMacros.h>
#undef ANKI_INSTANTIATE_RESOURCE
#undef ANKI_INSTANSIATE_RESOURCE_DELIMITER

} // end namespace anki
