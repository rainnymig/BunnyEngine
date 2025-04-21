#pragma once

#include <vulkan/vulkan.h>

namespace Bunny::Render
{
class ComputePipelineBuilder
{
  public:
    void setShader(VkShaderModule computeShader);
    void setPipelineLayout(VkPipelineLayout layout);
    VkPipeline build(VkDevice device);

  private:
    VkPipelineLayout mPipelineLayout;
    VkPipelineShaderStageCreateInfo mShaderStage;
};
} // namespace Bunny::Render
