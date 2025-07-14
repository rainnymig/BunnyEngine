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
    void addBinding(VkDescriptorSetLayoutBinding binding);
    void clear();
    VkDescriptorSetLayout build(
        VkDevice device, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags layoutCreateFlags = 0) const;

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

    void init(VkDevice device, uint32_t maxSets, std::span<PoolSize> poolSizes);
    void allocate(VkDevice device, VkDescriptorSetLayout* pLayout, VkDescriptorSet* pDescSet, uint32_t count = 1,
        void* pNext = nullptr);
    void clearPools(VkDevice device);
    void destroyPools(VkDevice device);

  private:
    VkDescriptorPool getPool(VkDevice device);
    VkDescriptorPool createPool(VkDevice device);

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
    ~DescriptorWriter();

    void clear();
    void writeImage(
        uint32_t binding, VkImageView imageView, VkSampler sampler, VkImageLayout layout, VkDescriptorType type);
    void writeBuffer(uint32_t binding, VkBuffer buffer, size_t size, size_t offset, VkDescriptorType type);
    void writeAccelerationStructure(uint32_t binding, VkAccelerationStructureKHR acceStruct);
    void writeImages(uint32_t binding, std::vector<VkDescriptorImageInfo> imageInfos, VkDescriptorType type);
    void writeBuffers(uint32_t binding, std::vector<VkDescriptorBufferInfo> bufferInfos, VkDescriptorType type);

    void updateSet(VkDevice device, VkDescriptorSet descriptorSet);

  private:
    std::deque<std::vector<VkDescriptorImageInfo>> mImageInfos;
    std::deque<std::vector<VkDescriptorBufferInfo>> mBufferInfos;
    std::deque<VkAccelerationStructureKHR> mAcceStructs;
    std::deque<VkWriteDescriptorSetAccelerationStructureKHR> mAcceStructWrites;

    std::vector<VkWriteDescriptorSet> mWrites;
};
} // namespace Bunny::Render
