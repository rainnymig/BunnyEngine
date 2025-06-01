#include "MaterialBank.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Descriptor.h"
#include "GraphicsPipelineBuilder.h"
#include "Error.h"

namespace Bunny::Render
{
void MaterialBank::cleanup()
{
    mMaterialInstances.clear();

    for (auto& material : mMaterials)
    {
        material->cleanup();
    }

    mMaterials.clear();
}

void MaterialBank::addMaterial(std::unique_ptr<Material>&& material)
{
    const IdType matId = mMaterials.size();
    mMaterials.emplace_back(std::move(material));
    mMaterials[matId]->mId = matId;
}

void MaterialBank::addMaterialInstance(MaterialInstance materialInstance)
{
    const IdType instId = mMaterialInstances.size();
    mMaterialInstances.emplace_back(materialInstance);
    mMaterialInstances[instId].mId = instId;
}

const Material* MaterialBank::getMaterial(IdType materialId) const
{
    return mMaterials.at(materialId).get();
}

const MaterialInstance& MaterialBank::getMaterialInstance(IdType instanceId) const
{
    return mMaterialInstances.at(instanceId);
}

PbrMaterialBank::PbrMaterialBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
}

BunnyResult PbrMaterialBank::initialize()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildPipelineLayouts())

    return BUNNY_HAPPY;
}

void PbrMaterialBank::cleanup()
{
    mDeletionStack.Flush();
}

void PbrMaterialBank::addMaterialInstance(const PbrMaterialParameters& materialParams)
{
}

void PbrMaterialBank::updateMaterialDescriptorSet(VkDescriptorSet descriptorSet) const
{
}

BunnyResult PbrMaterialBank::buildDescriptorSetLayouts()
{
    VkDescriptorSetLayoutBinding uniformBufferBinding{
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
    VkDescriptorSetLayoutBinding storageBufferBinding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr};

    //  split image and sampler later?
    VkDescriptorSetLayoutBinding imageBinding{
        0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

    DescriptorLayoutBuilder builder;

    builder.addBinding(uniformBufferBinding);
    uniformBufferBinding.binding = 1;
    builder.addBinding(uniformBufferBinding);
    uniformBufferBinding.binding = 1;
    uniformBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    builder.addBinding(uniformBufferBinding);
    mSceneDescSetLayout = builder.build(mVulkanResources->getDevice());

    builder.clear();
    builder.addBinding(storageBufferBinding);
    storageBufferBinding.binding = 1;
    builder.addBinding(storageBufferBinding);
    mObjectDescSetLayout = builder.build(mVulkanResources->getDevice());

    builder.clear();
    builder.addBinding(imageBinding);
    imageBinding.binding = 1;
    builder.addBinding(imageBinding);
    imageBinding.binding = 2;
    builder.addBinding(imageBinding);
    mGBufferDescSetLayout = builder.build(mVulkanResources->getDevice());

    builder.clear();
    storageBufferBinding.binding = 0;
    storageBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    builder.addBinding(storageBufferBinding);
    imageBinding.binding = 1;
    imageBinding.descriptorCount = TEXTURE_ARRAY_SIZE;
    builder.addBinding(imageBinding);
    mMaterialDescSetLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction([this]() {
        VkDevice device = mVulkanResources->getDevice();
        vkDestroyDescriptorSetLayout(device, mSceneDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mObjectDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mGBufferDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mMaterialDescSetLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

BunnyResult PbrMaterialBank::buildPipelineLayouts()
{
    {
        VkDescriptorSetLayout layouts[] = {mSceneDescSetLayout, mObjectDescSetLayout, mMaterialDescSetLayout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 3;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPbrForwardPipelineLayout);
    }

    {
        VkDescriptorSetLayout layouts[] = {mSceneDescSetLayout, mObjectDescSetLayout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 2;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPbrGBufferPipelineLayout);
    }

    {
        VkDescriptorSetLayout layouts[] = {mSceneDescSetLayout, mGBufferDescSetLayout, mMaterialDescSetLayout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 3;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        vkCreatePipelineLayout(
            mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPbrDeferredPipelineLayout);
    }

    mDeletionStack.AddFunction([this]() {
        VkDevice device = mVulkanResources->getDevice();
        vkDestroyPipelineLayout(device, mPbrForwardPipelineLayout, nullptr);
        vkDestroyPipelineLayout(device, mPbrGBufferPipelineLayout, nullptr);
        vkDestroyPipelineLayout(device, mPbrDeferredPipelineLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
