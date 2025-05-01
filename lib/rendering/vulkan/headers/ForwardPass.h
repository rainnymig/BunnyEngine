#pragma once

#include "Vertex.h"
#include "MeshBank.h"
#include "Descriptor.h"
#include "Fundamentals.h"

#include <vulkan/vulkan.h>

#include <array>
#include <unordered_map>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class MaterialBank;

class ForwardPass
{
  public:
    ForwardPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const MaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank);

    void initializePass(
        VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout objectLayout, VkDescriptorSetLayout drawLayout);
    void buildDrawCommands();
    void updateDrawInstanceCounts(std::unordered_map<IdType, size_t> meshInstanceCounts);
    void resetDrawCommands();
    void linkSceneData(const AllocatedBuffer& sceneBuffer);
    void linkLightData(const AllocatedBuffer& lightBuffer);
    void linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);
    void draw();
    void cleanup();

    const AllocatedBuffer& getDrawCommandBuffer() const { return mDrawCommandsBuffer; }
    const size_t getDrawCommandBufferSize() const;
    const AllocatedBuffer& getInstanceObjectBuffer() const { return mInstanceObjectBuffer; }
    const size_t getInstanceObjectBufferSize() const { return mInstanceObjectBufferSize; }

  private:
    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
    const MaterialBank* mMaterialBank;
    const MeshBank<NormalVertex>* mMeshBank;

    AllocatedBuffer mDrawCommandsBuffer;
    std::vector<VkDrawIndexedIndirectCommand> mDrawCommandsData;
    AllocatedBuffer mInstanceObjectBuffer;
    size_t mInstanceObjectBufferSize;

    VkDescriptorSetLayout mSceneDescLayout;
    VkDescriptorSetLayout mObjectDescLayout;
    VkDescriptorSetLayout mDrawDescLayout;
    DescriptorAllocator mDescriptorAllocator;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mSceneDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mObjectDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mDrawDataDescSets;
};

} // namespace Bunny::Render
