#include "PbrGraphicsPass.h"

#include "Error.h"
#include "ErrorCheck.h"
#include "ComputePipelineBuilder.h"

#include <cassert>

namespace Bunny::Render
{
PbrGraphicsPass::PbrGraphicsPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer),
      mMaterialBank(materialBank),
      mMeshBank(meshBank)
{
}

PbrGraphicsPass::~PbrGraphicsPass()
{
    cleanup();
}

BunnyResult PbrGraphicsPass::initializePass()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptors())
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initPipeline())
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDataAndResources())

    return BUNNY_HAPPY;
}

void PbrGraphicsPass::cleanup()
{
    mDeletionStack.Flush();
}

BunnyResult PbrGraphicsPass::initPipeline()
{
    return BUNNY_HAPPY;
}
BunnyResult PbrGraphicsPass::initDescriptors()
{
    return BUNNY_HAPPY;
}
BunnyResult PbrGraphicsPass::initDataAndResources()
{
    return BUNNY_HAPPY;
}

BunnyResult PbrGraphicsPass::buildComputePipeline(std::string_view shaderPath,
    const std::vector<VkDescriptorSetLayout>* descSetLayouts, const std::vector<VkPushConstantRange>* pushConstants,
    VkPipelineLayout* layout, VkPipeline* pipeline)
{
    VkDevice device = mVulkanResources->getDevice();
    Shader spectrumShader(shaderPath, device);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    if (descSetLayouts != nullptr)
    {
        pipelineLayoutInfo.setLayoutCount = descSetLayouts->size();
        pipelineLayoutInfo.pSetLayouts = descSetLayouts->data();
    }
    else
    {
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;
    }

    if (pushConstants != nullptr)
    {
        pipelineLayoutInfo.pushConstantRangeCount = pushConstants->size();
        pipelineLayoutInfo.pPushConstantRanges = pushConstants->data();
    }
    else
    {
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
    }

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, layout))
    mDeletionStack.AddFunction(
        [this, layout]() { vkDestroyPipelineLayout(mVulkanResources->getDevice(), *layout, nullptr); });

    ComputePipelineBuilder pipelineBuilder;
    pipelineBuilder.setShader(spectrumShader.getShaderModule());
    pipelineBuilder.setPipelineLayout(*layout);
    *pipeline = pipelineBuilder.build(device);

    mDeletionStack.AddFunction(
        [this, pipeline]() { vkDestroyPipeline(mVulkanResources->getDevice(), *pipeline, nullptr); });

    return BUNNY_HAPPY;
}
} // namespace Bunny::Render
