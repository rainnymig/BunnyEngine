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
    VkExtent3D mExtent;
    VkFormat mFormat;
};

struct AllocatedBuffer
{
    VkBuffer mBuffer;
    VmaAllocation mAllocation;
    VmaAllocationInfo mAllocationInfo;
};

//  frame
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

using IdType = size_t;

} // namespace Bunny::Render