#pragma once

#include "Fundamentals.h"
#include "Descriptor.h"

#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;

class DepthReducePass
{
  public:
    DepthReducePass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    void initializePass();
    void cleanup();
    void dispatch();

    const AllocatedImage& getDepthHierarchyImage() const { return mdepthHierarchyImage; }
    VkSampler getDepthReduceSampler() const { return mDepthReduceSampler; }
    uint32_t getDepthImageWidth() const { return mDepthWidth; }
    uint32_t getDepthImageHeight() const { return mDepthHeight; }
    uint32_t getDepthHierarchyLevels() const { return mDepthHierarchyLevels; }

  private:
    void createDepthHierarchy();
    void createDepthReduceSampler();
    void initDescriptorSets();
    void initPipeline();

    uint32_t findHierarchyLevels(uint32_t width, uint32_t height) const;
    uint32_t getGroupCount(uint32_t threadCount, uint32_t localSize) const;

    const VulkanRenderResources* mVulkanResources = nullptr;
    const VulkanGraphicsRenderer* mRenderer = nullptr;

    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;
    VkDescriptorSetLayout mDepthDescLayout;

    static constexpr size_t MAX_DEPTH_HIERARCHY_LEVELS = 16;
    AllocatedImage mdepthHierarchyImage;
    std::array<VkImageView, MAX_DEPTH_HIERARCHY_LEVELS> mDepthImageViewMips{nullptr};
    VkSampler mDepthReduceSampler;
    std::array<std::array<VkDescriptorSet, MAX_DEPTH_HIERARCHY_LEVELS>, MAX_FRAMES_IN_FLIGHT> mDepthDescSets;

    uint32_t mDepthWidth;
    uint32_t mDepthHeight;
    uint32_t mDepthHierarchyLevels;

    DescriptorAllocator mDescriptorAllocator;

    std::string mShaderPath{"./reduce_depth_comp.spv"};
};
} // namespace Bunny::Render
