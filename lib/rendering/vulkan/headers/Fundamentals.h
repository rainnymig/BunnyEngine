#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#include <limits>

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
    VkBuffer mBuffer = nullptr;
    VmaAllocation mAllocation;
    VmaAllocationInfo mAllocationInfo;
};

//  frame
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

using IdType = uint32_t;

//  wrapping this std::numeric_limits::max in parenthesis because windows defines min/max macro and will mess up things
//  here don't want to use /NOMINMAX
//  see https://stackoverflow.com/a/13566433
static constexpr IdType BUNNY_INVALID_ID = (std::numeric_limits<IdType>::max)();

} // namespace Bunny::Render