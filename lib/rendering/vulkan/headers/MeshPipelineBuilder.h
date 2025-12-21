#pragma once

#include <volk.h>

#include <vector>

namespace Bunny::Render
{
class MeshPipelineBuilder
{
  public:
    void setMeshShader(VkShaderModule meshShader);
    void setTaskShader(VkShaderModule taskShader);
    void setFragmentShader(VkShaderModule fragmentShader);
    void setPipelineLayout(VkPipelineLayout layout);
    VkPipeline build(VkDevice device);

  private:
    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
    VkPipelineLayout mPipelineLayout;
};
} // namespace Bunny::Render
