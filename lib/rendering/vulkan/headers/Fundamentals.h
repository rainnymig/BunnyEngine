#pragma once

#include <volk.h>
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
	bool mIsOwning = true;   //  temporary hack flag to indicate whether this image is owning the underlying resource
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
// static constexpr IdType BUNNY_INVALID_ID = (std::numeric_limits<IdType>::max)();
//  114514
static constexpr IdType BUNNY_INVALID_ID = 114514;

} // namespace Bunny::Render