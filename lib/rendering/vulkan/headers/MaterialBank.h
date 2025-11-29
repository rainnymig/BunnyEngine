#pragma once

#include "Fundamentals.h"
#include "Material.h"
#include "BunnyResult.h"
#include "FunctionStack.h"
#include "MeshBank.h"
#include "Vertex.h"

#include <memory>
#include <unordered_map>
#include <string>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;
class TextureBank;

class PbrMaterialBank
{
  public:
    //  Size of the array containing all textures in the shader
    //  This value is needed when creating the descriptor set
    //  For now this is fixed, try to make it growable when necessary
    //  In this case need to rebuild the whole rendering pipeline
    static constexpr uint32_t TEXTURE_ARRAY_SIZE = 64;

    PbrMaterialBank(
        const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer, TextureBank* textureBank);

    BunnyResult initialize();
    void cleanup();

    IdType giveMeAMaterial() const;
    IdType giveMeAMaterialInstance() const;

    PbrMaterialParameters getMaterialInstance(IdType id) const;
    IdType getRandomMaterialInstanceId() const;
    BunnyResult addMaterialInstance(const PbrMaterialParameters& materialParams, IdType& outId);
    //  temp solution, include mesh bank as parameter
    //  maybe order the descriptors better to avoid this
    void updateMaterialDescriptorSet(VkDescriptorSet descriptorSet, const MeshBank<NormalVertex>* meshBank) const;
    BunnyResult recreateMaterialBuffer();
    void updateMaterialBuffer();

    VkDescriptorSetLayout getWorldDescSetLayout() const { return mWorldDescSetLayout; }
    VkDescriptorSetLayout getObjectDescSetLayout() const { return mObjectDescSetLayout; }
    VkDescriptorSetLayout getGBufferDescSetLayout() const { return mGBufferDescSetLayout; }
    VkDescriptorSetLayout getMaterialDescSetLayout() const { return mMaterialDescSetLayout; }
    VkDescriptorSetLayout getEffectDescSetLayout() const { return mEffectDescSetLayout; }

    VkPipelineLayout getPbrForwardPipelineLayout() const { return mPbrForwardPipelineLayout; }
    VkPipelineLayout getPbrGBufferPipelineLayout() const { return mPbrGBufferPipelineLayout; }
    VkPipelineLayout getPbrDeferredPipelineLayout() const { return mPbrDeferredPipelineLayout; }

  private:
    BunnyResult buildDescriptorSetLayouts();
    BunnyResult buildPipelineLayouts();

    void showImguiControlPanel();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
    TextureBank* mTextureBank;

    VkDescriptorSetLayout mWorldDescSetLayout;
    VkDescriptorSetLayout mObjectDescSetLayout;
    VkDescriptorSetLayout mGBufferDescSetLayout;
    VkDescriptorSetLayout mMaterialDescSetLayout;
    VkDescriptorSetLayout mEffectDescSetLayout;

    VkPipelineLayout mPbrForwardPipelineLayout;
    VkPipelineLayout mPbrGBufferPipelineLayout;
    VkPipelineLayout mPbrDeferredPipelineLayout;

    std::vector<PbrMaterialParameters> mMaterialInstances;
    AllocatedBuffer mMaterialBuffer;
    bool mMaterialBufferNeedUpdate = false;

    Base::FunctionStack<> mDeletionStack;
};
} // namespace Bunny::Render
