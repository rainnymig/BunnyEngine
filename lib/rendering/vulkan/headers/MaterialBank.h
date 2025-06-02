#pragma once

#include "Fundamentals.h"
#include "Material.h"
#include "BunnyResult.h"
#include "FunctionStack.h"

#include <memory>
#include <unordered_map>
#include <string>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;
class TextureBank;

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
    struct PbrMaterialLoadParams
    {
        glm::vec4 mBaseColor;
        glm::vec4 mEmissiveColor;
        float mMetallic;
        float mRoughness;
        float mReflectance;
        float mAmbientOcclusion;
        std::string mColorTexPath;
        std::string mNormalTexPath;
        std::string mEmissiveTexPath;
        std::string mMetRouRflAmbTexPath;
    };

    PbrMaterialBank(
        const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer, TextureBank* textureBank);

    BunnyResult initialize();
    void cleanup();

    BunnyResult addMaterialInstance(const PbrMaterialLoadParams& materialParams, IdType& outId);
    void updateMaterialDescriptorSet(VkDescriptorSet descriptorSet);

    VkDescriptorSetLayout getSceneDescSetLayout() const { return mSceneDescSetLayout; }
    VkDescriptorSetLayout getObjectDescSetLayout() const { return mObjectDescSetLayout; }
    VkDescriptorSetLayout getGBufferDescSetLayout() const { return mGBufferDescSetLayout; }
    VkDescriptorSetLayout getMaterialDescSetLayout() const { return mMaterialDescSetLayout; }

    VkPipelineLayout getPbrForwardPipelineLayout() const { return mPbrForwardPipelineLayout; }
    VkPipelineLayout getPbrGBufferPipelineLayout() const { return mPbrGBufferPipelineLayout; }
    VkPipelineLayout getPbrDeferredPipelineLayout() const { return mPbrDeferredPipelineLayout; }

  private:
    //  Size of the array containing all textures in the shader
    //  This value is needed when creating the descriptor set
    //  For now this is fixed, try to make it growable when necessary
    //  In this case need to rebuild the whole rendering pipeline
    static constexpr uint32_t TEXTURE_ARRAY_SIZE = 64;

    BunnyResult buildDescriptorSetLayouts();
    BunnyResult buildPipelineLayouts();
    BunnyResult recreateMaterialBuffer();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
    TextureBank* mTextureBank;

    VkDescriptorSetLayout mSceneDescSetLayout;
    VkDescriptorSetLayout mObjectDescSetLayout;
    VkDescriptorSetLayout mGBufferDescSetLayout;
    VkDescriptorSetLayout mMaterialDescSetLayout;

    VkPipelineLayout mPbrForwardPipelineLayout;
    VkPipelineLayout mPbrGBufferPipelineLayout;
    VkPipelineLayout mPbrDeferredPipelineLayout;

    std::vector<PbrMaterialParameters> mMaterialInstances;
    AllocatedBuffer mMaterialBuffer;
    bool mMaterialBufferNeedUpdate = false;

    Base::FunctionStack<> mDeletionStack;
};
} // namespace Bunny::Render
