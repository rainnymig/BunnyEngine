#pragma once

#include "Fundamentals.h"
#include "Material.h"
#include "BunnyResult.h"
#include "FunctionStack.h"

#include <memory>
#include <unordered_map>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;

class MaterialBank
{
  public:
    void cleanup();

    void addMaterial(std::unique_ptr<Material>&& material);
    void addMaterialInstance(MaterialInstance materialInstance);

    const Material* getMaterial(IdType materialId) const;
    const MaterialInstance& getMaterialInstance(IdType instanceId) const;

    //  helper function to return a default material
    constexpr IdType getDefaultMaterialId() const { return 0; }
    constexpr IdType getDefaultMaterialInstanceId() const { return 0; }

  private:
    std::vector<std::unique_ptr<Material>> mMaterials;
    std::vector<MaterialInstance> mMaterialInstances;
};

class PbrMaterialBank
{
  public:
    PbrMaterialBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    BunnyResult initialize();
    void cleanup();

  private:
    //  Size of the array containing all textures in the shader
    //  This value is needed when creating the descriptor set
    //  For now this is fixed, try to make it growable when necessary
    //  In this case need to rebuild the whole rendering pipeline
    static constexpr uint32_t TEXTURE_ARRAY_SIZE = 32;

    BunnyResult buildDescriptorSetLayouts();
    BunnyResult buildPipelineLayouts();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;

    std::vector<PbrMaterialParameters> mMaterialInstances;

    VkDescriptorSetLayout mSceneDescSetLayout;
    VkDescriptorSetLayout mObjectDescSetLayout;
    VkDescriptorSetLayout mGBufferDescSetLayout;
    VkDescriptorSetLayout mMaterialDescSetLayout;

    VkPipelineLayout mPbrForwardPipelineLayout;
    VkPipelineLayout mPbrGBufferPipelineLayout;
    VkPipelineLayout mPbrDeferredPipelineLayout;

    VkPipeline mPbrGBufferPipeline;

    std::vector<VkPipeline> mPbrForwardPipelines;
    std::vector<VkPipeline> mPbrDeferredPipelines;

    Base::FunctionStack<> mDeletionStack;
};
} // namespace Bunny::Render
