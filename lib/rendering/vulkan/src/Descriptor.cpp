#include "Descriptor.h"

#include "ErrorCheck.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace Bunny::Render
{
void DescriptorLayoutBuilder::addBinding(VkDescriptorSetLayoutBinding binding)
{
    mBindings.push_back(binding);
}

void DescriptorLayoutBuilder::clear()
{
    mBindings.clear();
}

VkDescriptorSetLayout DescriptorLayoutBuilder::build(
    VkDevice device, void* pNext, VkDescriptorSetLayoutCreateFlags layoutCreateFlags) const
{
    VkDescriptorSetLayout layout = nullptr;

    if (mBindings.empty())
    {
        return layout;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = static_cast<uint32_t>(mBindings.size());
    layoutInfo.pBindings = mBindings.data();
    layoutInfo.flags = layoutCreateFlags;
    layoutInfo.pNext = pNext;

    VK_HARD_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout));

    return layout;
}

void DescriptorAllocator::init(VkDevice device, uint32_t maxSets, std::span<PoolSize> poolSizes)
{
    assert(maxSets > 0 && !poolSizes.empty());

    mMaxSets = std::min(maxSets, msMaxSetsLimit);

    for (const PoolSize& ps : poolSizes)
    {
        mPoolSizes.emplace_back(ps);
    }

    VkDescriptorPool newPool = createPool(device);
    mReadyPools.push_back(newPool);
}

void DescriptorAllocator::allocate(
    VkDevice device, VkDescriptorSetLayout* pLayout, VkDescriptorSet* pDescSet, uint32_t count, void* pNext)
{
    VkDescriptorPool pool = getPool(device);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool;
    allocInfo.descriptorSetCount = count;
    allocInfo.pSetLayouts = pLayout;
    allocInfo.pNext = pNext;

    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, pDescSet);

    //  if these error occur it means pool is full
    //  push the pool to full pools and create new pool
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
    {
        mFullPools.push_back(pool);
        pool = createPool(device);
        allocInfo.descriptorPool = pool;

        //  if it fails again then panic
        VK_HARD_CHECK(vkAllocateDescriptorSets(device, &allocInfo, pDescSet));
    }

    mReadyPools.push_back(pool);
}

void DescriptorAllocator::clearPools(VkDevice device)
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

void DescriptorAllocator::destroyPools(VkDevice device)
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

VkDescriptorPool DescriptorAllocator::getPool(VkDevice device)
{
    VkDescriptorPool newPool;
    if (mReadyPools.empty())
    {
        newPool = createPool(device);
        return newPool;
    }
    else
    {
        newPool = mReadyPools.back();
        mReadyPools.pop_back();
        return newPool;
    }
}

VkDescriptorPool DescriptorAllocator::createPool(VkDevice device)
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

DescriptorWriter::~DescriptorWriter()
{
    clear();
}

void DescriptorWriter::clear()
{
    mImageInfos.clear();
    mBufferInfos.clear();
    mWrites.clear();
}

void DescriptorWriter::writeImage(
    uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type)
{
    writeImages(binding,
        std::vector<VkDescriptorImageInfo>{
            {sampler, imageView, layout}
    },
        type);
}

void DescriptorWriter::writeBuffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type)
{
    writeBuffers(binding,
        std::vector<VkDescriptorBufferInfo>{
            {buffer, offset, size}
    },
        type);
}

void DescriptorWriter::writeImages(
    uint32_t binding, std::vector<VkDescriptorImageInfo> imageInfos, VkDescriptorType type)
{
    const std::vector<VkDescriptorImageInfo>& infos = mImageInfos.emplace_back(std::move(imageInfos));

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = static_cast<uint32_t>(infos.size()),
        .descriptorType = type,
        .pImageInfo = infos.data()};
}

void DescriptorWriter::writeBuffers(
    uint32_t binding, std::vector<VkDescriptorBufferInfo> bufferInfos, VkDescriptorType type)
{
    const std::vector<VkDescriptorBufferInfo>& infos = mBufferInfos.emplace_back(std::move(bufferInfos));

    VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .pNext = nullptr,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = static_cast<uint32_t>(infos.size()),
        .descriptorType = type,
        .pBufferInfo = infos.data()};
}

void DescriptorWriter::updateSet(VkDevice device, VkDescriptorSet descriptorSet)
{
    for (VkWriteDescriptorSet& write : mWrites)
    {
        write.dstSet = descriptorSet;
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(mWrites.size()), mWrites.data(), 0, nullptr);
}

} // namespace Bunny::Render
