#pragma once

#include <volk.h>

namespace Bunny
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
} // namespace Bunny
