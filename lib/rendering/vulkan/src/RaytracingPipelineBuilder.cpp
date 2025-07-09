#include "RaytracingPipelineBuilder.h"

#include "Helper.h"

namespace Bunny::Render
{

VkPipeline RaytracingPipelineBuilder::build(VkDevice device)
{
    VkRayTracingPipelineCreateInfoKHR createInfo{.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
    createInfo.stageCount = mShaderStages.size();
    createInfo.pStages = mShaderStages.data();
    createInfo.groupCount = mShaderGroups.size();
    createInfo.pGroups = mShaderGroups.data();
    createInfo.maxPipelineRayRecursionDepth = mMaxRecursionDepth;
    createInfo.layout = mPipelineLayout;

    VkPipeline pipeline;
    vkCreateRayTracingPipelinesKHR(device, {}, {}, 1, &createInfo, nullptr, &pipeline);

    return pipeline;
}

void RaytracingPipelineBuilder::clear()
{
    mShaderStages.clear();
    mShaderGroups.clear();
    mPipelineLayout = nullptr;
    mMaxRecursionDepth = 1;
}

uint32_t RaytracingPipelineBuilder::addShaderStage(VkShaderModule shader, VkShaderStageFlagBits stage)
{
    uint32_t newStageIdx = mShaderStages.size();
    mShaderStages.push_back(makeShaderStageCreateInfo(stage, shader));
    return newStageIdx;
}

void RaytracingPipelineBuilder::addGeneralShaderGroup(uint32_t index)
{
    VkRayTracingShaderGroupCreateInfoKHR createInfo = makeRayTracingShaderGroupCreateInfoKHR();
    createInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    createInfo.generalShader = index;
    mShaderGroups.push_back(createInfo);
}

void RaytracingPipelineBuilder::addClosestHitShaderGroup(uint32_t index, VkRayTracingShaderGroupTypeKHR type)
{
    assert(type == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR ||
           type == VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR);

    VkRayTracingShaderGroupCreateInfoKHR createInfo = makeRayTracingShaderGroupCreateInfoKHR();
    createInfo.type = type;
    createInfo.closestHitShader = index;
    mShaderGroups.push_back(createInfo);
}

void RaytracingPipelineBuilder::addAnyHitShaderGroup(uint32_t index, VkRayTracingShaderGroupTypeKHR type)
{
    assert(type == VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR ||
           type == VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR);

    VkRayTracingShaderGroupCreateInfoKHR createInfo = makeRayTracingShaderGroupCreateInfoKHR();
    createInfo.type = type;
    createInfo.anyHitShader = index;
    mShaderGroups.push_back(createInfo);
}

void RaytracingPipelineBuilder::addIntersectionShaderGroup(uint32_t index)
{
    VkRayTracingShaderGroupCreateInfoKHR createInfo = makeRayTracingShaderGroupCreateInfoKHR();
    createInfo.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
    createInfo.intersectionShader = index;
    mShaderGroups.push_back(createInfo);
}

void RaytracingPipelineBuilder::setMaxRecursionDepth(uint32_t depth)
{
    mMaxRecursionDepth = depth;
}

} // namespace Bunny::Render
