#pragma once

#include "Fundamentals.h"
#include "Descriptor.h"

#include <volk.h>

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

    const std::array<const AllocatedImage*, MAX_FRAMES_IN_FLIGHT> getDepthHierarchyImages() const;
    VkSampler getDepthReduceSampler() const { return mDepthReduceSampler; }
    uint32_t getDepthImageWidth() const { return mDepthWidth; }
    uint32_t getDepthImageHeight() const { return mDepthHeight; }
    uint32_t getDepthHierarchyLevels() const { return mDepthHierarchyLevels; }

  private:
    static constexpr size_t MAX_DEPTH_HIERARCHY_LEVELS = 16;

    struct FrameData
    {
        AllocatedImage mdepthHierarchyImage;
        std::array<VkImageView, MAX_DEPTH_HIERARCHY_LEVELS> mDepthImageViewMips{VK_NULL_HANDLE};

        std::array<VkDescriptorSet, MAX_DEPTH_HIERARCHY_LEVELS> mDepthDescSet;
    };

    void createDepthHierarchy();
    void createDepthReduceSampler();
    void initDescriptorSets();
    void initPipeline();

    uint32_t findHierarchyLevels(uint32_t width, uint32_t height) const;
    uint32_t getGroupCount(uint32_t threadCount, uint32_t localSize) const;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;

    const VulkanRenderResources* mVulkanResources = nullptr;
    const VulkanGraphicsRenderer* mRenderer = nullptr;

    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;
    VkDescriptorSetLayout mDepthDescLayout;

    VkSampler mDepthReduceSampler;

    uint32_t mDepthWidth;
    uint32_t mDepthHeight;
    uint32_t mDepthHierarchyLevels;

    DescriptorAllocator mDescriptorAllocator;

    std::string mShaderPath{"./reduce_depth_comp.spv"};
};
} // namespace Bunny::Render
