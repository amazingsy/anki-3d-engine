// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

ANKI_CONFIG_OPTION(gr_validation, 0, 0, 1)
ANKI_CONFIG_OPTION(gr_debugPrintf, 0, 0, 1)
ANKI_CONFIG_OPTION(gr_debugMarkers, 0, 0, 1)
ANKI_CONFIG_OPTION(gr_vsync, 0, 0, 1)
ANKI_CONFIG_OPTION(gr_maxBindlessTextures, 256, 8, 1024)
ANKI_CONFIG_OPTION(gr_maxBindlessImages, 32, 8, 1024)
ANKI_CONFIG_OPTION(gr_rayTracing, 0, 0, 1, "Try enabling ray tracing")
ANKI_CONFIG_OPTION(gr_64bitAtomics, 1, 0, 1)
ANKI_CONFIG_OPTION(gr_samplerFilterMinMax, 1, 0, 1)

// Vulkan
ANKI_CONFIG_OPTION(gr_diskShaderCacheMaxSize, 128_MB, 1_MB, 1_GB)
ANKI_CONFIG_OPTION(gr_vkminor, 1, 1, 1)
ANKI_CONFIG_OPTION(gr_vkmajor, 1, 1, 1)
ANKI_CONFIG_OPTION(gr_asyncCompute, 1, 0, 1, "Enable or not async compute")
