#include "ComputePipelineBuilder.h"

#include "Helper.h"

namespace Bunny::Render
{
void ComputePipelineBuilder::setShader(VkShaderModule computeShader)
{
    mShaderStage = makeShaderStageCreateInfo(VK_SHADER_STAGE_COMPUTE_BIT, computeShader);
}

void ComputePipelineBuilder::setPipelineLayout(VkPipelineLayout layout)
{
    mPipelineLayout = layout;
}

VkPipeline ComputePipelineBuilder::build(VkDevice device)
{
    VkComputePipelineCreateInfo pipelineInfo{.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .stage = mShaderStage,
        .layout = mPipelineLayout};

    VkPipeline newPipeline;
    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    }
    else
    {
        return newPipeline;
    }
}

} // namespace Bunny::Render
