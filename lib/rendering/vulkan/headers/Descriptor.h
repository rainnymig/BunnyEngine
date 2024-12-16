#pragma once

#include <vulkan/vulkan.h>

#include <span>
#include <vector>
#include <deque>

namespace Bunny::Render
{
class DescriptorLayoutBuilder
{
  public:
    void AddBinding(VkDescriptorSetLayoutBinding binding);
    void Clear();
    VkDescriptorSetLayout Build(VkDevice device) const;

  private:
    std::vector<VkDescriptorSetLayoutBinding> mBindings;
};

class DescriptorAllocator
{
  public:
    struct PoolSize
    {
        VkDescriptorType mType;

        //  this is the ratio of descriptor count in VkDescriptorSize
        //  and max sets in descriptor pool create info
        uint32_t mRatio;
    };

    void Init(VkDevice device, uint32_t maxSets, std::span<PoolSize> poolSizes);
    void Allocate(VkDevice device, VkDescriptorSetLayout* pLayout, VkDescriptorSet* pDescSet, uint32_t count = 1,
        void* pNext = nullptr);
    void ClearPools(VkDevice device);
    void DestroyPools(VkDevice device);

  private:
    VkDescriptorPool GetPool(VkDevice device);
    VkDescriptorPool CreatePool(VkDevice device);

    std::vector<VkDescriptorPool> mFullPools;
    std::vector<VkDescriptorPool> mReadyPools;
    std::vector<PoolSize> mPoolSizes;

    uint32_t mMaxSets;
    constexpr static uint32_t msMaxSetsLimit = 4096;
};

//  Helper to write the actual resources to descriptors
class DescriptorWriter
{
  public:
    void Clear();
    void WriteImage(
        uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void WriteBuffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);

    void UpdateSet(VkDevice device, VkDescriptorSet descriptorSet);

  private:
    std::deque<VkDescriptorImageInfo> mImageInfos;
    std::deque<VkDescriptorBufferInfo> mBufferInfos;

    std::vector<VkWriteDescriptorSet> mWrites;
};
} // namespace Bunny::Render
