#include "MeshPipelineBuilder.h"

#include "Helper.h"

namespace Bunny::Render
{
void MeshPipelineBuilder::setMeshShader(VkShaderModule meshShader)
{
    mShaderStages.push_back(makeShaderStageCreateInfo(VK_SHADER_STAGE_MESH_BIT_EXT, meshShader));
}

void MeshPipelineBuilder::setTaskShader(VkShaderModule taskShader)
{
    mShaderStages.push_back(makeShaderStageCreateInfo(VK_SHADER_STAGE_TASK_BIT_EXT, taskShader));
}

void MeshPipelineBuilder::setFragmentShader(VkShaderModule fragmentShader)
{
    mShaderStages.push_back(makeShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void MeshPipelineBuilder::setPipelineLayout(VkPipelineLayout layout)
{
    mPipelineLayout = layout;
}

VkPipeline MeshPipelineBuilder::build(VkDevice device)
{
    VkGraphicsPipelineCreateInfo pipelineInfo = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    pipelineInfo.stageCount = mShaderStages.size();
    pipelineInfo.pStages = mShaderStages.data();
    pipelineInfo.layout = mPipelineLayout;

    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    }
    else
    {
        return newPipeline;
    }
}

} // namespace Bunny::Render
