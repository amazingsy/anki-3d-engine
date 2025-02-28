// Copyright (C) 2009-2021, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <AnKi/Gr/Common.h>

#if ANKI_OS_LINUX
#	define VK_USE_PLATFORM_XCB_KHR 1
#	define VK_USE_PLATFORM_XLIB_KHR 1
#elif ANKI_OS_WINDOWS
#	define VK_USE_PLATFORM_WIN32_KHR 1
#elif ANKI_OS_ANDROID
#	define VK_USE_PLATFORM_ANDROID_KHR 1
#else
#	error Not implemented
#endif
#include <Volk/volk.h>

// Cleanup global namespace from these dirty libaries
#if ANKI_OS_LINUX
#	undef Bool
#elif ANKI_OS_WINDOWS
#	include <AnKi/Util/CleanupWindows.h>
#endif

namespace anki {

// Forward
class GrManagerImpl;

/// @addtogroup vulkan
/// @{

#define ANKI_VK_LOGI(...) ANKI_LOG("VK  ", NORMAL, __VA_ARGS__)
#define ANKI_VK_LOGE(...) ANKI_LOG("VK  ", ERROR, __VA_ARGS__)
#define ANKI_VK_LOGW(...) ANKI_LOG("VK  ", WARNING, __VA_ARGS__)
#define ANKI_VK_LOGF(...) ANKI_LOG("VK  ", FATAL, __VA_ARGS__)

#define ANKI_VK_SELF(class_) class_& self = *static_cast<class_*>(this)
#define ANKI_VK_SELF_CONST(class_) const class_& self = *static_cast<const class_*>(this)

enum class DescriptorType : U8
{
	COMBINED_TEXTURE_SAMPLER,
	TEXTURE,
	SAMPLER,
	UNIFORM_BUFFER,
	STORAGE_BUFFER,
	IMAGE,
	TEXTURE_BUFFER,
	ACCELERATION_STRUCTURE,

	COUNT
};

enum class VulkanExtensions : U32
{
	NONE = 0,
	KHR_XCB_SURFACE = 1 << 1,
	KHR_XLIB_SURFACE = 1 << 2,
	KHR_WIN32_SURFACE = 1 << 3,
	KHR_ANDROID_SURFACE = 1 << 4,
	KHR_SWAPCHAIN = 1 << 5,
	KHR_SURFACE = 1 << 6,
	EXT_DEBUG_MARKER = 1 << 7,
	EXT_DEBUG_REPORT = 1 << 8,
	AMD_SHADER_INFO = 1 << 9,
	AMD_RASTERIZATION_ORDER = 1 << 10,
	KHR_RAY_TRACING = 1 << 11,
	KHR_PIPELINE_EXECUTABLE_PROPERTIES = 1 << 12,
	EXT_DESCRIPTOR_INDEXING = 1 << 13,
	KHR_BUFFER_DEVICE_ADDRESS = 1 << 14,
	EXT_SCALAR_BLOCK_LAYOUT = 1 << 15,
	KHR_TIMELINE_SEMAPHORE = 1 << 16,
	KHR_SHADER_FLOAT16_INT8 = 1 << 17,
	KHR_SHADER_ATOMIC_INT64 = 1 << 18,
	KHR_SPIRV_1_4 = 1 << 19,
	KHR_SHADER_FLOAT_CONTROLS = 1 << 20,
	EXT_SAMPLER_FILTER_MIN_MAX = 1 << 21,
};
ANKI_ENUM_ALLOW_NUMERIC_OPERATIONS(VulkanExtensions)

enum class VulkanQueueType : U8
{
	GENERAL,
	COMPUTE,

	COUNT,
	FIRST = 0
};
ANKI_ENUM_ALLOW_NUMERIC_OPERATIONS(VulkanQueueType)

using VulkanQueueFamilies = Array<U32, U32(VulkanQueueType::COUNT)>;

/// @name Constants
/// @{
constexpr U DESCRIPTOR_POOL_INITIAL_SIZE = 64;
constexpr F32 DESCRIPTOR_POOL_SIZE_SCALE = 2.0f;
constexpr U DESCRIPTOR_FRAME_BUFFERING = 60 * 5; ///< How many frames worth of descriptors to buffer.

/// There is no need to ask for a fence or a semaphore to be waited for more than 10 seconds. The GPU will timeout
/// anyway.
constexpr Second MAX_FENCE_OR_SEMAPHORE_WAIT_TIME = 10.0;
/// @}

/// Some internal buffer usage flags.
class PrivateBufferUsageBit
{
public:
	static constexpr BufferUsageBit ACCELERATION_STRUCTURE_BUILD_SCRATCH = BufferUsageBit(1ull << 29ull);
	static constexpr BufferUsageBit ACCELERATION_STRUCTURE = static_cast<BufferUsageBit>(1ull << 30ull);

	static constexpr BufferUsageBit ALL_PRIVATE = ACCELERATION_STRUCTURE_BUILD_SCRATCH | ACCELERATION_STRUCTURE;
};
static_assert(!(BufferUsageBit::ALL & PrivateBufferUsageBit::ALL_PRIVATE), "Update the bits in PrivateBufferUsageBit");

/// Check if a vulkan function failed. It will abort on failure.
#define ANKI_VK_CHECKF(x) \
	do \
	{ \
		VkResult rez; \
		if(ANKI_UNLIKELY((rez = (x)) < 0)) \
		{ \
			ANKI_VK_LOGF("Vulkan function failed (VkResult: %s): %s", vkResultToString(rez), #x); \
		} \
	} while(0)

/// Check if a vulkan function failed.
#define ANKI_VK_CHECK(x) \
	do \
	{ \
		VkResult rez; \
		if(ANKI_UNLIKELY((rez = (x)) < 0)) \
		{ \
			ANKI_VK_LOGE("Vulkan function failed (VkResult: %s): %s", vkResultToString(rez), #x); \
			return Error::FUNCTION_FAILED; \
		} \
	} while(0)

/// Convert compare op.
ANKI_USE_RESULT VkCompareOp convertCompareOp(CompareOperation ak);

/// Convert format.
ANKI_USE_RESULT inline VkFormat convertFormat(const Format ak)
{
	ANKI_ASSERT(ak != Format::NONE);
	const VkFormat out = static_cast<VkFormat>(ak);
	return out;
}

/// Get format aspect mask.
ANKI_USE_RESULT inline DepthStencilAspectBit getImageAspectFromFormat(const Format ak)
{
	DepthStencilAspectBit out = DepthStencilAspectBit::NONE;
	if(formatIsStencil(ak))
	{
		out = DepthStencilAspectBit::STENCIL;
	}

	if(formatIsDepth(ak))
	{
		out |= DepthStencilAspectBit::DEPTH;
	}

	return out;
}

/// Convert image aspect.
ANKI_USE_RESULT inline VkImageAspectFlags convertImageAspect(const DepthStencilAspectBit ak)
{
	VkImageAspectFlags out = 0;
	if(!!(ak & DepthStencilAspectBit::DEPTH))
	{
		out |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if(!!(ak & DepthStencilAspectBit::STENCIL))
	{
		out |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	if(!out)
	{
		out = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	return out;
}

/// Convert topology.
ANKI_USE_RESULT VkPrimitiveTopology convertTopology(PrimitiveTopology ak);

/// Convert fill mode.
ANKI_USE_RESULT VkPolygonMode convertFillMode(FillMode ak);

/// Convert cull mode.
ANKI_USE_RESULT VkCullModeFlags convertCullMode(FaceSelectionBit ak);

/// Convert blend method.
ANKI_USE_RESULT VkBlendFactor convertBlendFactor(BlendFactor ak);

/// Convert blend function.
ANKI_USE_RESULT VkBlendOp convertBlendOperation(BlendOperation ak);

/// Convert color write mask.
inline ANKI_USE_RESULT VkColorComponentFlags convertColorWriteMask(ColorBit ak)
{
	return static_cast<VkColorComponentFlags>(ak);
}

/// Convert load op.
ANKI_USE_RESULT VkAttachmentLoadOp convertLoadOp(AttachmentLoadOperation ak);

/// Convert store op.
ANKI_USE_RESULT VkAttachmentStoreOp convertStoreOp(AttachmentStoreOperation ak);

/// Convert buffer usage bitmask.
ANKI_USE_RESULT VkBufferUsageFlags convertBufferUsageBit(BufferUsageBit usageMask);

ANKI_USE_RESULT VkImageType convertTextureType(TextureType ak);

ANKI_USE_RESULT VkImageViewType convertTextureViewType(TextureType ak);

ANKI_USE_RESULT VkImageUsageFlags convertTextureUsage(const TextureUsageBit ak, const Format format);

ANKI_USE_RESULT VkStencilOp convertStencilOp(StencilOperation ak);

ANKI_USE_RESULT VkShaderStageFlags convertShaderTypeBit(ShaderTypeBit bit);

ANKI_USE_RESULT inline VkVertexInputRate convertVertexStepRate(VertexStepRate ak)
{
	VkVertexInputRate out;
	switch(ak)
	{
	case VertexStepRate::VERTEX:
		out = VK_VERTEX_INPUT_RATE_VERTEX;
		break;
	case VertexStepRate::INSTANCE:
		out = VK_VERTEX_INPUT_RATE_INSTANCE;
		break;
	default:
		ANKI_ASSERT(0);
		out = VK_VERTEX_INPUT_RATE_INSTANCE;
	}
	return out;
}

ANKI_USE_RESULT inline VkDescriptorType convertDescriptorType(DescriptorType ak)
{
	VkDescriptorType out;
	switch(ak)
	{
	case DescriptorType::COMBINED_TEXTURE_SAMPLER:
		out = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		break;
	case DescriptorType::TEXTURE:
		out = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		break;
	case DescriptorType::SAMPLER:
		out = VK_DESCRIPTOR_TYPE_SAMPLER;
		break;
	case DescriptorType::UNIFORM_BUFFER:
		out = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		break;
	case DescriptorType::STORAGE_BUFFER:
		out = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		break;
	case DescriptorType::IMAGE:
		out = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		break;
	case DescriptorType::ACCELERATION_STRUCTURE:
		out = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		break;
	default:
		out = VK_DESCRIPTOR_TYPE_MAX_ENUM;
		ANKI_ASSERT(0);
	}

	return out;
}

ANKI_USE_RESULT inline VkIndexType convertIndexType(IndexType ak)
{
	VkIndexType out;
	switch(ak)
	{
	case IndexType::U16:
		out = VK_INDEX_TYPE_UINT16;
		break;
	case IndexType::U32:
		out = VK_INDEX_TYPE_UINT32;
		break;
	default:
		ANKI_ASSERT(0);
		out = VK_INDEX_TYPE_MAX_ENUM;
	}

	return out;
}

ANKI_USE_RESULT inline VkRasterizationOrderAMD convertRasterizationOrder(RasterizationOrder ak)
{
	VkRasterizationOrderAMD out;
	switch(ak)
	{
	case RasterizationOrder::ORDERED:
		out = VK_RASTERIZATION_ORDER_STRICT_AMD;
		break;
	case RasterizationOrder::RELAXED:
		out = VK_RASTERIZATION_ORDER_RELAXED_AMD;
		break;
	default:
		ANKI_ASSERT(0);
		out = VK_RASTERIZATION_ORDER_STRICT_AMD;
	}

	return out;
}

ANKI_USE_RESULT inline VkAccelerationStructureTypeKHR convertAccelerationStructureType(AccelerationStructureType ak)
{
	VkAccelerationStructureTypeKHR out;
	switch(ak)
	{
	case AccelerationStructureType::BOTTOM_LEVEL:
		out = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		break;
	case AccelerationStructureType::TOP_LEVEL:
		out = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		break;
	default:
		ANKI_ASSERT(0);
		out = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	}

	return out;
}

ANKI_USE_RESULT const char* vkResultToString(VkResult res);
/// @}

} // end namespace anki
