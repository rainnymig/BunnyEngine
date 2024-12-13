#include "Descriptor.h"

#include "ErrorCheck.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace Bunny::Render
{
void DescriptorLayoutBuilder::AddBinding(VkDescriptorSetLayoutBinding binding)
{
    mBindings.push_back(binding);
}

void DescriptorLayoutBuilder::Clear()
{
    mBindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device) const
{
    VkDescriptorSetLayout layout = nullptr;

    if (mBindings.empty())
    {
        return layout;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(mBindings.size());
    layoutInfo.pBindings = mBindings.data();

    VK_HARD_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));

    return layout;
}

void DescriptorAllocator::Init(VkDevice device, uint32_t maxSets, std::span<PoolSize> poolSizes)
{
    assert(maxSets > 0 && !poolSizes.empty());

    mMaxSets = std::min(maxSets, msMaxSetsLimit);

    for (const PoolSize& ps : poolSizes)
    {
        mPoolSizes.emplace_back(ps);
    }

    VkDescriptorPool newPool = CreatePool(device);
    mReadyPools.push_back(newPool);
}

VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout, void* pNext)
{
    VkDescriptorPool pool = GetPool(device);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;
    allocInfo.pNext = pNext;

    VkDescriptorSet newSet;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &newSet);

    //  if these error occur it means pool is full
    //  push the pool to full pools and create new pool
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        mFullPools.push_back(pool);
        pool = CreatePool(device);
        allocInfo.descriptorPool = pool;

        //  if it fails again then panic
        VK_HARD_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &newSet));
    }

    mReadyPools.push_back(pool);

    return newSet;
}

void DescriptorAllocator::ClearPools(VkDevice device)
{
    for (auto p : mReadyPools)
    {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto p : mFullPools)
    {
        vkResetDescriptorPool(device, p, 0);
    }
    mFullPools.clear();
}

void DescriptorAllocator::DestroyPools(VkDevice device)
{
    for (auto p : mReadyPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    mReadyPools.clear();
    for (auto p : mFullPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    mFullPools.clear();
}

VkDescriptorPool DescriptorAllocator::GetPool(VkDevice device)
{
    VkDescriptorPool newPool;
    if (mReadyPools.empty())
    {
        newPool = CreatePool(device);
        return newPool;
    }
    else
    {
        newPool = mReadyPools.back();
        mReadyPools.pop_back();
        return newPool;
    }
}

VkDescriptorPool DescriptorAllocator::CreatePool(VkDevice device)
{
    assert(mMaxSets > 0 && mMaxSets <= msMaxSetsLimit && !mPoolSizes.empty());
    std::vector<VkDescriptorPoolSize> descPoolSizes;
    descPoolSizes.reserve(mPoolSizes.size());
    std::transform(mPoolSizes.begin(), mPoolSizes.end(), std::back_inserter(descPoolSizes), [this](const PoolSize& ps) {
        return VkDescriptorPoolSize{.type = ps.mType, .descriptorCount = ps.mRatio * mMaxSets};
    });

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = 0;
    poolInfo.maxSets = mMaxSets;
    poolInfo.poolSizeCount = (uint32_t)descPoolSizes.size();
    poolInfo.pPoolSizes = descPoolSizes.data();

    VkDescriptorPool newPool;
    VK_HARD_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool));
    if (newPool != nullptr)
    {
        mMaxSets = std::min(static_cast<uint32_t>(mMaxSets * 1.5), msMaxSetsLimit);
    }
    return newPool;
}

void DescriptorWriter::Clear()
{
    mImageInfos.clear();
    mBufferInfos.clear();
    mWrites.clear();
}

void DescriptorWriter::WriteImage(
    int binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    VkDescriptorImageInfo& info = mImageInfos.emplace_back(
        VkDescriptorImageInfo{.sampler = sampler, .imageView = imageView, .imageLayout = layout});

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .pImageInfo = &info};

    mWrites.push_back(write);
}

void DescriptorWriter::WriteBuffer(int binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& info =
        mBufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = buffer, .offset = offset, .range = size});

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .pBufferInfo = &info};

    mWrites.push_back(write);
}

void DescriptorWriter::UpdateSet(VkDevice device, VkDescriptorSet descriptorSet)
{
    for (VkWriteDescriptorSet write : mWrites)
    {
        write.dstSet = descriptorSet;
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(mWrites.size()), mWrites.data(), 0, nullptr);
}

} // namespace Bunny::Render
