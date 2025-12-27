#pragma once

#include <volk.h>

#include <vector>

namespace Bunny::Render
{

//  This class is a helper for building a graphics VkPipeline
//  Mostly taken from the similar class in [Vulkan Guide](https://vkguide.dev/docs/new_chapter_3/building_pipeline/)
class GraphicsPipelineBuilder
{
  public:
    enum class PipelineType
    {
        Vertex,
        Mesh
    };

    GraphicsPipelineBuilder();
    VkPipeline build(VkDevice device, PipelineType pipelineType = PipelineType::Vertex);
    void clear();

    void addShaderStage(VkShaderModule shader, VkShaderStageFlagBits stage);
    void setInputTopology(VkPrimitiveTopology topology);
    void setVertexInput(const VkVertexInputAttributeDescription* pAttributeDesc, uint32_t attributeDescCount,
        const VkVertexInputBindingDescription* pBindingDesc, uint32_t bindingDescCount);
    void setPolygonMode(VkPolygonMode mode);
    void setCulling(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void setMultisamplingNone();
    void disableBlending();
    void enableBlendingAdditive();
    void enableBlendingAlphablend();
    void setColorAttachmentFormats(const std::vector<VkFormat>& formats);
    void setDepthFormat(VkFormat format);
    void disableDepthTest();
    void enableDepthTest(bool depthWriteEnable, VkCompareOp op);
    void setPipelineLayout(VkPipelineLayout layout);

  private:
    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
    VkPipelineVertexInputStateCreateInfo mVertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo mInputAssembly;
    VkPipelineRasterizationStateCreateInfo mRasterizer;
    VkPipelineColorBlendAttachmentState mColorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo mMultisampling;
    VkPipelineLayout mPipelineLayout;
    VkPipelineDepthStencilStateCreateInfo mDepthStencil;
    VkPipelineRenderingCreateInfo mRenderInfo;
    std::vector<VkFormat> mColorAttachmentFormats;
};
} // namespace Bunny::Render
