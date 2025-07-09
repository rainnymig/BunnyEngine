#include "RaytracingShadowPass.h"

#include "Error.h"
#include "ErrorCheck.h"
#include "MaterialBank.h"
#include "Shader.h"
#include "RaytracingPipelineBuilder.h"

#include <vulkan/vulkan.h>

#include <array>

namespace Bunny::Render
{
RaytracingShadowPass::RaytracingShadowPass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank)
    : super(vulkanResources, renderer, materialBank, meshBank)
{
}

void RaytracingShadowPass::draw() const
{
}

BunnyResult RaytracingShadowPass::initPipeline()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildPipelineLayout())

    VkDevice device = mVulkanResources->getDevice();
    Shader rayGenShader("rtbasic_rgen.sprv", device);
    Shader closestHitShader("rtshadow_chit.sprv", device);
    Shader basicMissShader("rtbasic_miss.sprv", device);
    Shader shadowMissShader("rtshadow_miss.sprv", device);

    RaytracingPipelineBuilder builder;
    uint32_t rayGenIdx = builder.addShaderStage(rayGenShader.getShaderModule(), VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    uint32_t basicMissIdx = builder.addShaderStage(basicMissShader.getShaderModule(), VK_SHADER_STAGE_MISS_BIT_KHR);
    uint32_t shadowMissIdx = builder.addShaderStage(shadowMissShader.getShaderModule(), VK_SHADER_STAGE_MISS_BIT_KHR);
    uint32_t closestHitIdx =
        builder.addShaderStage(closestHitShader.getShaderModule(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

    builder.addGeneralShaderGroup(rayGenIdx);
    builder.addGeneralShaderGroup(basicMissIdx);
    builder.addGeneralShaderGroup(shadowMissIdx);
    builder.addClosestHitShaderGroup(closestHitIdx);

    builder.setMaxRecursionDepth(2); //  1 camera ray + 1 shadow ray
                                     //  need more if multiple light?

    mPipeline = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction([this]() {
        if (mPipeline != nullptr)
        {
            vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr);
            mPipeline = nullptr;
        }
    });

    return BUNNY_HAPPY;
}

BunnyResult RaytracingShadowPass::buildPipelineLayout()
{
    //  take descriptor set layouts from material bank and build pipeline layout
    //  need to also add descriptor set layouts for vertex and index buffer as storage buffers
    std::array<VkDescriptorSetLayout, 3> descLayouts{mMaterialBank->getSceneDescSetLayout(),
        mMaterialBank->getObjectDescSetLayout(), mMaterialBank->getMaterialDescSetLayout()};
    VkPipelineLayoutCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    createInfo.pushConstantRangeCount = 0;
    createInfo.pPushConstantRanges = nullptr;
    createInfo.setLayoutCount = descLayouts.size();
    createInfo.pSetLayouts = descLayouts.data();

    VK_CHECK_OR_RETURN_BUNNY_SAD(
        vkCreatePipelineLayout(mVulkanResources->getDevice(), &createInfo, nullptr, &mPipelineLayout))

    mDeletionStack.AddFunction([this]() {
        if (mPipelineLayout != nullptr)
        {
            vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr);
            mPipelineLayout = nullptr;
        }
    });

    return BUNNY_HAPPY;
}

BunnyResult Render::RaytracingShadowPass::buildRaytracingDescSetLayouts()
{
    //  build descriptor set layouts that are specific to the ray tracing pipeline
    //  such as desc sets for acceleration structures, vertex buffer, index buffer and output image

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
