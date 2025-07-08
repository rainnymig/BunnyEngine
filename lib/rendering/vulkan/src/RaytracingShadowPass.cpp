#include "RaytracingShadowPass.h"

#include "Error.h"
#include "ErrorCheck.h"
#include "MaterialBank.h"

#include <vulkan/vulkan.h>

#include <array>

namespace Bunny::Render
{
RaytracingShadowPass::RaytracingShadowPass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
    std::string_view raygenShader, std::string_view closestHitShader, std::string_view missShader)
    : super(vulkanResources, renderer, materialBank, meshBank),
      mRaygenShaderPath(raygenShader),
      mClosestHitShaderPath(closestHitShader),
      mMissShaderPath(missShader)
{
}

void Render::RaytracingShadowPass::draw() const
{
}

BunnyResult Render::RaytracingShadowPass::initPipeline()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildPipelineLayout())

    mDeletionStack.AddFunction([this]() {
        if (mPipeline != nullptr)
        {
            vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr);
            mPipeline = nullptr;
        }
    });

    return BUNNY_HAPPY;
}

BunnyResult Render::RaytracingShadowPass::buildPipelineLayout()
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

} // namespace Bunny::Render
