#include "MaterialBank.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Descriptor.h"
#include "GraphicsPipelineBuilder.h"
#include "Error.h"
#include "TextureBank.h"

#include <random>
#include <cassert>

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

IdType MaterialBank::giveMeAMaterial() const
{
    return getDefaultMaterialId();
}

IdType MaterialBank::giveMeAMaterialInstance() const
{
    return getDefaultMaterialInstanceId();
}

PbrMaterialBank::PbrMaterialBank(
    const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer, TextureBank* textureBank)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer),
      mTextureBank(textureBank)
{
}

BunnyResult PbrMaterialBank::initialize()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildDescriptorSetLayouts())
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildPipelineLayouts())

    return BUNNY_HAPPY;
}

void PbrMaterialBank::cleanup()
{
    mVulkanResources->destroyBuffer(mMaterialBuffer);
    mDeletionStack.Flush();
}

IdType PbrMaterialBank::giveMeAMaterial() const
{
    return getRandomMaterialInstanceId();
}

IdType PbrMaterialBank::giveMeAMaterialInstance() const
{
    return getRandomMaterialInstanceId();
}

PbrMaterialParameters PbrMaterialBank::getMaterialInstance(IdType id) const
{
    return mMaterialInstances.at(id);
}

IdType PbrMaterialBank::getRandomMaterialInstanceId() const
{
    static std::random_device rd;
    static std::mt19937 re(rd());

    std::uniform_int_distribution<int> uniDist(0, mMaterialInstances.size() - 1);
    return uniDist(re);
}

BunnyResult PbrMaterialBank::addMaterialInstance(const PbrMaterialLoadParams& materialParams, IdType& outId)
{
    outId = BUNNY_INVALID_ID;

    PbrMaterialParameters materialInstance{materialParams.mBaseColor, materialParams.mEmissiveColor,
        materialParams.mMetallic, materialParams.mRoughness, materialParams.mReflectance,
        materialParams.mAmbientOcclusion, BUNNY_INVALID_ID, BUNNY_INVALID_ID, BUNNY_INVALID_ID, BUNNY_INVALID_ID};

    //  may need to review the image format later
    if (!materialParams.mColorTexPath.empty())
    {
        BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mTextureBank->addTexture(
            materialParams.mColorTexPath.c_str(), VK_FORMAT_R8G8B8A8_SRGB, materialInstance.mColorTexId))
    }
    if (!materialParams.mNormalTexPath.empty())
    {
        BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mTextureBank->addTexture(
            materialParams.mNormalTexPath.c_str(), VK_FORMAT_R32G32B32_SFLOAT, materialInstance.mNormalTexId))
    }
    if (!materialParams.mEmissiveTexPath.empty())
    {
        BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mTextureBank->addTexture(
            materialParams.mEmissiveTexPath.c_str(), VK_FORMAT_R8G8B8A8_SRGB, materialInstance.mEmissiveTexId))
    }
    if (!materialParams.mMetRouRflAmbTexPath.empty())
    {
        BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mTextureBank->addTexture(
            materialParams.mMetRouRflAmbTexPath.c_str(), VK_FORMAT_R8G8B8A8_UNORM, materialInstance.mMetRouRflAmbTexId))
    }

    outId = mMaterialInstances.size();
    mMaterialInstances.push_back(materialInstance);

    mMaterialBufferNeedUpdate = true;

    return BUNNY_HAPPY;
}

void PbrMaterialBank::updateMaterialDescriptorSet(VkDescriptorSet descriptorSet) const
{
    assert(!mMaterialBufferNeedUpdate);

    DescriptorWriter writer;
    writer.writeBuffer(0, mMaterialBuffer.mBuffer, mMaterialInstances.size() * sizeof(PbrMaterialParameters), 0,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    mTextureBank->addDescriptorSetWrite(1, writer);
    writer.updateSet(mVulkanResources->getDevice(), descriptorSet);
}

BunnyResult PbrMaterialBank::recreateMaterialBuffer()
{
    //  maybe need to wait for current rendering to finish?

    //  clean existing material buffer
    mVulkanResources->destroyBuffer(mMaterialBuffer);

    //  and then recreate using new data
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mVulkanResources->createBufferWithData(mMaterialInstances.data(),
        mMaterialInstances.size() * sizeof(PbrMaterialParameters), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_MEMORY_USAGE_AUTO, mMaterialBuffer))

    mMaterialBufferNeedUpdate = false;

    return BUNNY_HAPPY;
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

    //  light data
    uniformBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    builder.addBinding(uniformBufferBinding);
    //  camera data
    uniformBufferBinding.binding = 1;
    uniformBufferBinding.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    builder.addBinding(uniformBufferBinding);
    mWorldDescSetLayout = builder.build(mVulkanResources->getDevice());

    builder.clear();
    //  object data
    storageBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    builder.addBinding(storageBufferBinding);
    //  instance to object id
    storageBufferBinding.binding = 1;
    builder.addBinding(storageBufferBinding);
    mObjectDescSetLayout = builder.build(mVulkanResources->getDevice());

    builder.clear();
    //  color map
    builder.addBinding(imageBinding);
    //  fragPos texU map
    imageBinding.binding = 1;
    builder.addBinding(imageBinding);
    //  normal texV map
    imageBinding.binding = 2;
    builder.addBinding(imageBinding);
    mGBufferDescSetLayout = builder.build(mVulkanResources->getDevice());

    builder.clear();
    //  all material data
    storageBufferBinding.binding = 0;
    storageBufferBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    builder.addBinding(storageBufferBinding);
    //  all textures array
    imageBinding.binding = 1;
    imageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    imageBinding.descriptorCount = TEXTURE_ARRAY_SIZE;
    builder.addBinding(imageBinding);
    mMaterialDescSetLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction([this]() {
        VkDevice device = mVulkanResources->getDevice();
        vkDestroyDescriptorSetLayout(device, mWorldDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mObjectDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mGBufferDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mMaterialDescSetLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

BunnyResult PbrMaterialBank::buildPipelineLayouts()
{
    {
        VkDescriptorSetLayout layouts[] = {mWorldDescSetLayout, mObjectDescSetLayout, mMaterialDescSetLayout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 3;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPbrForwardPipelineLayout);
    }

    {
        VkDescriptorSetLayout layouts[] = {mWorldDescSetLayout, mObjectDescSetLayout};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 2;
        pipelineLayoutInfo.pSetLayouts = layouts;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPbrGBufferPipelineLayout);
    }

    {
        VkDescriptorSetLayout layouts[] = {mWorldDescSetLayout, mGBufferDescSetLayout, mMaterialDescSetLayout};
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
