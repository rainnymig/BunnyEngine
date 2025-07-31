#pragma once

#include "Transform.h"
#include "Fundamentals.h"
#include "Descriptor.h"
#include "BunnyResult.h"
#include "MeshBank.h"
#include "Vertex.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <volk.h>

#include <vector>
#include <array>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;
class Camera;

class CullingPass
{
  public:
    CullingPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const MeshBank<NormalVertex>* meshBank);

    BunnyResult initializePass();
    void cleanup();
    void linkDrawData(const AllocatedBuffer& drawCommandBuffer, size_t drawbufferSize,
        const AllocatedBuffer& instObjectBuffer, size_t instBufferSize);
    void linkMeshData(const AllocatedBuffer& meshDataBuffer, size_t bufferSize);
    void linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);
    void linkCullingData(const AllocatedImage& depthHierarchyImage, VkSampler sampler);
    void setObjectCount(uint32_t objectCount);
    void setDepthImageSizes(uint32_t width, uint32_t height, uint32_t levels);
    void updateCullingData(const Camera& camera);
    void dispatch();

    ~CullingPass();

  private:
    void initDescriptorSets();
    BunnyResult initPipeline();
    void createBuffers();

    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;

    DescriptorAllocator mDescriptorAllocator;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mObjectDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mCullDataDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mDrawCommandDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mMeshDataDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mDebugDataDescSets;
    VkDescriptorSetLayout mStorageBufferLayout;
    VkDescriptorSetLayout mCullDataLayout;
    VkDescriptorSetLayout mDrawDataLayout;

    AllocatedBuffer mCullingDataBuffer;
    const AllocatedBuffer* mDrawCommandBuffer = nullptr;
    const AllocatedBuffer* mInstanceObjectBuffer = nullptr;
    uint32_t mObjectCount = 0;
    uint32_t mDepthImageWidth;
    uint32_t mDepthImageHeight;
    uint32_t mDepthMipLevels;

    AllocatedBuffer mDebugDataBuffer;

    const VulkanRenderResources* mVulkanResources = nullptr;
    const VulkanGraphicsRenderer* mRenderer = nullptr;
    const MeshBank<NormalVertex>* mMeshBank = nullptr;

    std::string mCullingShaderPath{"./culling_comp.spv"};
};
} // namespace Bunny::Render
