#pragma once

#include "IdType.h"

#include <volk.h>
#include <vk_mem_alloc.h>

#include <limits>

namespace Bunny
{
struct AllocatedImage
{
    VkImage mImage;
    VkImageView mImageView;
    VmaAllocation mAllocation;
    VkExtent3D mExtent;
    VkFormat mFormat;
    bool mIsOwning = true; //  temporary hack flag to indicate whether this image is owning the underlying resource
};

struct AllocatedBuffer
{
    VkBuffer mBuffer = nullptr;
    VmaAllocation mAllocation;
    VmaAllocationInfo mAllocationInfo;
    VkDeviceSize mSize;
};

//  frame
static constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

} // namespace Bunny