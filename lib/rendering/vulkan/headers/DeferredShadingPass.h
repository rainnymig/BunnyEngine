#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"
#include "BunnyResult.h"
#include "Vertex.h"

#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;

class DeferredShadingPass
{
  public:
    DeferredShadingPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    void initializePass();
    void linkLightData(const AllocatedBuffer& lightBuffer);
    void linkGBufferMaps(const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>& colorMaps,
        const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>& fragPosMaps,
        const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>& normalTexCoordMaps);
    void draw();
    void cleanup();

  private:
    void initDescriptorSets();
    void createSampler();
    BunnyResult initPipeline();
    void buildScreenQuad();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;

    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;

    AllocatedBuffer mVertexBuffer;
    AllocatedBuffer mIndexBuffer;
    std::array<ScreenQuadVertex, 4> mVertexData;
    std::array<uint32_t, 6> mIndexData;

    VkSampler mGbufferImageSampler;
    const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>* mColorMaps = nullptr;
    const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>* mFragPosMaps = nullptr;
    const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>* mNormalTexCoordMaps = nullptr;

    DescriptorAllocator mDescriptorAllocator;
    VkDescriptorSetLayout mUniformDescLayout;
    VkDescriptorSetLayout mGBufferDescLayout;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mLightDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mGBufferDescSets; //  for the gbuffer images

    std::string mVertexShaderPath{"./screen_quad_vert.spv"};
    std::string mFragmentShaderPath{"./basic_deferred_frag.spv"};
};
} // namespace Bunny::Render
