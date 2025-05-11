#include "GraphicsPipelineBuilder.h"

#include "Helper.h"

namespace Bunny::Render
{
GraphicsPipelineBuilder::GraphicsPipelineBuilder()
{
    clear();
}

VkPipeline GraphicsPipelineBuilder::build(VkDevice device)
{
    //  make viewport state from our stored viewport and scissor.
    //  at the moment we wont support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    //  setup dummy color blending. We arent using transparent objects yet
    //  the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = mRenderInfo.colorAttachmentCount;
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachments(
        mRenderInfo.colorAttachmentCount, mColorBlendAttachment);
    colorBlending.pAttachments = blendAttachments.data();

    //  build the actual pipeline
    //  we now use all of the info structs we have been writing into into this one
    //  to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    //  connect the renderInfo to the pNext extension mechanism
    pipelineInfo.pNext = &mRenderInfo;

    pipelineInfo.stageCount = (uint32_t)mShaderStages.size();
    pipelineInfo.pStages = mShaderStages.data();
    pipelineInfo.pVertexInputState = &mVertexInputInfo;
    pipelineInfo.pInputAssemblyState = &mInputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &mRasterizer;
    pipelineInfo.pMultisampleState = &mMultisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &mDepthStencil;
    pipelineInfo.layout = mPipelineLayout;

    //  viewport state and scissor state are dynamic so that we don't need to recreate the pipeline when these two
    //  changes
    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicInfo.pDynamicStates = &state[0];
    dynamicInfo.dynamicStateCount = 2;

    pipelineInfo.pDynamicState = &dynamicInfo;

    //  build the pipeline!
    VkPipeline newPipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        return VK_NULL_HANDLE; // failed to create graphics pipeline
    }
    else
    {
        return newPipeline;
    }
}

void GraphicsPipelineBuilder::clear()
{
    mVertexInputInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    mInputAssembly = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    mRasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    mColorAttachmentFormats.clear();
    mMultisampling = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    mPipelineLayout = {};
    mDepthStencil = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    mRenderInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    mShaderStages.clear();
}

void GraphicsPipelineBuilder::setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
{
    mShaderStages.clear();
    mShaderStages.push_back(makeShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
    mShaderStages.push_back(makeShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
}

void GraphicsPipelineBuilder::setInputTopology(VkPrimitiveTopology topology)
{
    mInputAssembly.topology = topology;

    //  if true, when using STRIP type topologies, it's possible to break up lines or triangles
    //  not needed for now
    mInputAssembly.primitiveRestartEnable = VK_FALSE;
}

void GraphicsPipelineBuilder::setVertexInput(const VkVertexInputAttributeDescription* pAttributeDesc,
    uint32_t attributeDescCount, const VkVertexInputBindingDescription* pBindingDesc, uint32_t bindingDescCount)
{
    mVertexInputInfo.pNext = nullptr;
    mVertexInputInfo.vertexBindingDescriptionCount = bindingDescCount;
    mVertexInputInfo.pVertexBindingDescriptions = pBindingDesc;
    mVertexInputInfo.vertexAttributeDescriptionCount = attributeDescCount;
    mVertexInputInfo.pVertexAttributeDescriptions = pAttributeDesc;
}

void GraphicsPipelineBuilder::setPolygonMode(VkPolygonMode mode)
{
    mRasterizer.polygonMode = mode;
    mRasterizer.lineWidth = 1.0f;
}

void GraphicsPipelineBuilder::setCulling(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    mRasterizer.cullMode = cullMode;
    mRasterizer.frontFace = frontFace;
}

void GraphicsPipelineBuilder::setMultisamplingNone()
{
    mMultisampling.sampleShadingEnable = VK_FALSE;
    // multisampling defaulted to no multisampling (1 sample per pixel)
    mMultisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    mMultisampling.minSampleShading = 1.0f;
    mMultisampling.pSampleMask = nullptr;
    // no alpha to coverage either
    mMultisampling.alphaToCoverageEnable = VK_FALSE;
    mMultisampling.alphaToOneEnable = VK_FALSE;
}

void GraphicsPipelineBuilder::disableBlending()
{
    // default write mask
    mColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // no blending
    mColorBlendAttachment.blendEnable = VK_FALSE;
}

void GraphicsPipelineBuilder::enableBlendingAdditive()
{
    mColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    mColorBlendAttachment.blendEnable = VK_TRUE;
    mColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    mColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    mColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    mColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    mColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    mColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void GraphicsPipelineBuilder::enableBlendingAlphablend()
{
    mColorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    mColorBlendAttachment.blendEnable = VK_TRUE;
    mColorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    mColorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    mColorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    mColorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    mColorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    mColorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
}

void GraphicsPipelineBuilder::setColorAttachmentFormats(const std::vector<VkFormat>& formats)
{
    mColorAttachmentFormats = formats;
    mRenderInfo.colorAttachmentCount = mColorAttachmentFormats.size();
    mRenderInfo.pColorAttachmentFormats = mColorAttachmentFormats.data();
}

void GraphicsPipelineBuilder::setDepthFormat(VkFormat format)
{
    mRenderInfo.depthAttachmentFormat = format;
}

void GraphicsPipelineBuilder::disableDepthTest()
{
    mDepthStencil.depthTestEnable = VK_FALSE;
    mDepthStencil.depthWriteEnable = VK_FALSE;
    mDepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    mDepthStencil.depthBoundsTestEnable = VK_FALSE;
    mDepthStencil.stencilTestEnable = VK_FALSE;
    mDepthStencil.front = {};
    mDepthStencil.back = {};
    mDepthStencil.minDepthBounds = 0.f;
    mDepthStencil.maxDepthBounds = 1.f;
}

void GraphicsPipelineBuilder::enableDepthTest(bool depthWriteEnable, VkCompareOp op)
{
    mDepthStencil.depthTestEnable = VK_TRUE;
    mDepthStencil.depthWriteEnable = depthWriteEnable;
    mDepthStencil.depthCompareOp = op;
    mDepthStencil.depthBoundsTestEnable = VK_FALSE;
    mDepthStencil.stencilTestEnable = VK_FALSE;
    mDepthStencil.front = {};
    mDepthStencil.back = {};
    mDepthStencil.minDepthBounds = 0.f;
    mDepthStencil.maxDepthBounds = 1.f;
}

void GraphicsPipelineBuilder::setPipelineLayout(VkPipelineLayout layout)
{
    mPipelineLayout = layout;
}

} // namespace Bunny::Render
