// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Gr/OcclusionQuery.h>
#include <AnKi/Gr/gl/GlObject.h>

namespace anki {

/// @addtogroup opengl
/// @{

/// Occlusion query.
class OcclusionQueryImpl final : public OcclusionQuery, public GlObject
{
public:
	OcclusionQueryImpl(GrManager* manager, CString name)
		: OcclusionQuery(manager, name)
	{
	}

	~OcclusionQueryImpl()
	{
		destroyDeferred(getManager(), glDeleteQueries);
	}

	/// Create the query.
	void init();

	/// Begin query.
	void begin();

	/// End query.
	void end();

	/// Get query result.
	OcclusionQueryResult getResult() const;
};
/// @}

} // end namespace anki
