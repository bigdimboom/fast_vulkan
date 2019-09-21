#pragma once
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.hpp>

namespace vkapi
{

struct BufferObject
{
	vk::Buffer buffer;
	VmaAllocation alloc_meta;
};

struct ImageObject
{
	vk::Image image;
	VmaAllocation alloc_meta;
};

} // end namespace vkapi