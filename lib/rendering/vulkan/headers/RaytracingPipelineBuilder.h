#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Bunny::Render
{
class RaytracingPipelineBuilder
{
  public:
    VkPipeline build(VkDevice device);
    void clear();

    //  add a shader stage and returns the stage index
    uint32_t addShaderStage(VkShaderModule shader, VkShaderStageFlagBits stage);
    void addGeneralShaderGroup(uint32_t index);
    void addClosestHitShaderGroup(
        uint32_t index, VkRayTracingShaderGroupTypeKHR type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);
    void addAnyHitShaderGroup(
        uint32_t index, VkRayTracingShaderGroupTypeKHR type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR);
    void addIntersectionShaderGroup(uint32_t index);
    void setMaxRecursionDepth(uint32_t depth);

  private:
    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> mShaderGroups;
    VkPipelineLayout mPipelineLayout = nullptr;
    uint32_t mMaxRecursionDepth = 1;
};
} // namespace Bunny::Render
