#pragma once

#include "Transform.h"
#include "Fundamentals.h"
#include "Descriptor.h"
#include "BunnyResult.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vulkan/vulkan.h>

#include <vector>
#include <array>

namespace Bunny::Render
{
class VulkanRenderResources;
class Camera;

class CullingPass
{
  public:
    CullingPass(const VulkanRenderResources* vulkanResources);

    BunnyResult initializePass();
    void cleanup();
    void createBuffers();
    void linkDrawData(const AllocatedBuffer& drawCommandBuffer, size_t bufferSize);
    void linkMeshData(const AllocatedBuffer& meshDataBuffer, size_t bufferSize);
    void linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);
    void updateCullingData(const Camera& camera);
    void dispatch();

    ~CullingPass();

  private:
    void initDescriptorSets();
    BunnyResult initPipeline();

    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;

    DescriptorAllocator mDescriptorAllocator;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mObjectDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mCullDataDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mDrawCommandDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mMeshDataDescSets;
    VkDescriptorSetLayout mStorageBufferLayout;
    VkDescriptorSetLayout mUniformBufferLayout;

    AllocatedBuffer mCullingDataBuffer;

    const VulkanRenderResources* mVulkanResources;
    std::string mCullingShaderPath{"./culling_comp.spv"};
};
} // namespace Bunny::Render
