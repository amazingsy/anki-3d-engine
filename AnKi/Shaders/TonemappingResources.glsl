// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// Tonemapping resources

#pragma once

#include <AnKi/Shaders/Common.glsl>

#ifndef TONEMAPPING_RESOURCE_AS_BUFFER
#	define TONEMAPPING_RESOURCE_AS_BUFFER 0
#endif

#if TONEMAPPING_RESOURCE_AS_BUFFER
layout(std140, set = TONEMAPPING_SET, binding = TONEMAPPING_BINDING) buffer b_tonemapping
#else
layout(std140, set = TONEMAPPING_SET, binding = TONEMAPPING_BINDING) uniform b_tonemapping
#endif
{
	Vec4 u_averageLuminanceExposurePad2;
};

#define u_averageLuminance u_averageLuminanceExposurePad2.x
#define u_exposureThreshold0 u_averageLuminanceExposurePad2.y
