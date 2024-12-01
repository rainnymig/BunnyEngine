#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Bunny::Render
{

//  This class is a helper for building a graphics VkPipeline
//  Mostly taken from the similar class in [Vulkan Guide](https://vkguide.dev/docs/new_chapter_3/building_pipeline/)
class PipelineBuilder
{
  public:
    PipelineBuilder();
    VkPipeline build(VkDevice device);
    void clear();

    void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode);
    void setCulling(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setMultisamplingNone();
    void disableBlending();
    void enableBlendingAdditive();
    void enableBlendingAlphablend();
    void setColorAttachmentFormat(VkFormat format);
    void setDepthFormat(VkFormat format);
    void disableDepthTest();
    void enableDepthTest(bool depthWriteEnable, VkCompareOp op);

  private:
    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
    VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
    VkPipelineRasterizationStateCreateInfo mRasterizer;
    VkPipelineColorBlendAttachmentState mColorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo mMultisampling;
    VkPipelineLayout mPipelineLayout;
    VkPipelineDepthStencilStateCreateInfo mDepthStencil;
    VkPipelineRenderingCreateInfo mRenderInfo;
    VkFormat mColorAttachmentformat;
};
} // namespace Bunny::Render
