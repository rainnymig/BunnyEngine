#pragma once

#include "RenderJob.h"
#include "Vertex.h"
#include "MeshBank.h"
#include "Descriptor.h"
#include "Fundamentals.h"

#include <vulkan/vulkan.h>

#include <array>

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

    void initializePass(VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout objectLayout);
    void updateSceneData(const AllocatedBuffer& sceneBuffer);
    void updateLightData(const AllocatedBuffer& lightBuffer);
    void updateObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);
    void renderBatch(const RenderBatch& batch);
    void renderAll();
    void cleanup();

  private:
    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
    const MaterialBank* mMaterialBank;
    const MeshBank<NormalVertex>* mMeshBank;

    VkDescriptorSetLayout mSceneDescLayout;
    VkDescriptorSetLayout mObjectDescLayout;
    DescriptorAllocator mDescriptorAllocator;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mSceneDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mObjectDescSets;
};

} // namespace Bunny::Render
