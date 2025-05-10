#pragma once

#include "Descriptor.h"
#include "Vertex.h"
#include "MeshBank.h"
#include "Fundamentals.h"
#include "BunnyResult.h"

#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class MaterialBank;

class GBufferPass
{
  public:
    GBufferPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const MeshBank<NormalVertex>* meshBank);

    void initializePass();
    void buildDrawCommands();
    void updateDrawInstanceCounts(std::unordered_map<IdType, size_t> meshInstanceCounts);
    void resetDrawCommands();
    void linkSceneData(const AllocatedBuffer& sceneBuffer);
    void linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);
    void draw();
    void cleanup();

  private:
    void initDescriptorSets();
    BunnyResult initPipeline();
    void createImages();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
    const MeshBank<NormalVertex>* mMeshBank;

    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;

    std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT> mColorMaps;
    std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT> mFragPosMaps;
    std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT> mNormalTexCoordMaps;

    AllocatedBuffer mDrawCommandsBuffer;
    AllocatedBuffer mInitialDrawCommandBuffer;
    std::vector<VkDrawIndexedIndirectCommand> mDrawCommandsData;
    AllocatedBuffer mInstanceObjectBuffer;
    size_t mInstanceObjectBufferSize;

    VkDescriptorSetLayout mUniformDescLayout;
    VkDescriptorSetLayout mStorageDescLayout;
    DescriptorAllocator mDescriptorAllocator;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mSceneDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mObjectDescSets;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mDrawDataDescSets;

    std::string mVertexShaderPath{"./culled_instanced_vert.spv"};
    std::string mFragmentShaderPath{"./gbuffer_frag.spv"};
};
} // namespace Bunny::Render
