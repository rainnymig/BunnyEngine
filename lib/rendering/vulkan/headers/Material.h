#pragma once

#include "Descriptor.h"

#include <vulkan/vulkan.h>
#include <string_view>

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
    void buildPipeline(VkDevice device, VkFormat colorAttachmentFormat, VkFormat depthFormat);
    void cleanupPipeline();

    MaterialInstance createInstance();

  private:
    static constexpr std::string_view VERTEX_SHADER_PATH {"./basic_updated_vert.spv"};
    static constexpr std::string_view FRAGMENT_SHADER_PATH {"./basic_updated_frag.spv"};

    void buildDescriptorSetLayout(VkDevice device);

    DescriptorAllocator mDescriptorAllocator;
    MaterialPipeline mPipeline;
    VkDevice mDevice;
    VkDescriptorSetLayout mDescriptorSetLayout;
};
} // namespace Bunny::Render
