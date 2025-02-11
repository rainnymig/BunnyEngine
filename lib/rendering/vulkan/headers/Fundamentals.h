#pragma once

#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>

namespace Bunny::Render
{
struct AllocatedImage
{
    VkImage mImage;
    VkImageView mImageView;
    VmaAllocation mAllocation;
    VkExtent3D mExtend;
    VkFormat mFormat;
};

struct AllocatedBuffer
{
    VkBuffer mBuffer;
    VmaAllocation mAllocation;
    VmaAllocationInfo mAllocationInfo;
};

} // namespace Bunny::Render