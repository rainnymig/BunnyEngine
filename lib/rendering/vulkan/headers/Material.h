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
    MaterialPipeline* mBaseMaterial;
    VkDescriptorSet mDescriptorSet;
};
} // namespace Bunny::Render
