#pragma once

#include <vulkan/vulkan.h>

namespace Bunny::Render
{
struct MaterialPipeline
{
    VkPipeline mPipiline;
    VkPipelineLayout mPipelineLayout;
};

struct MaterialInstance
{
    MaterialPipeline* mpBaseMaterial;
    VkDescriptorSet mDescriptorSet;
};

class BasicBlinnPhongMaterial
{
  public:
  private:
    MaterialPipeline mPipeline;
};
} // namespace Bunny::Render
