// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Renderer/RendererObject.h>

namespace anki {

/// @addtogroup renderer
/// @{

/// Forward rendering stage. The objects that blend must be handled differently
class ForwardShading : public RendererObject
{
public:
	ForwardShading(Renderer* r)
		: RendererObject(r)
	{
	}

	~ForwardShading();

	ANKI_USE_RESULT Error init(const ConfigSet& initializer);

	void setDependencies(const RenderingContext& ctx, GraphicsRenderPassDescription& pass);

	void run(const RenderingContext& ctx, RenderPassWorkContext& rgraphCtx);
};
/// @}

} // end namespace anki
