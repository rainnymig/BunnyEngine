#include "PbrForwardPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "MaterialBank.h"
#include "MeshBank.h"
#include "Shader.h"
#include "GraphicsPipelineBuilder.h"

namespace Bunny::Render
{
void PbrForwardPass::draw() const
{
    static constexpr bool updateDepth = true;
    mRenderer->beginRender(updateDepth);

    mRenderer->finishRender();
}

BunnyResult PbrForwardPass::initPipeline()
{
    VkDevice device = mVulkanResources->getDevice();

    //  load shader file
    Shader vertexShader(mVertexShaderPath, device);
    Shader fragmentShader(mFragmentShaderPath, device);

    mPipelineLayout = mMaterialBank->getPbrForwardPipelineLayout();

    //  vertex info
    auto bindingDescription = getBindingDescription<NormalVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = NormalVertex::getAttributeDescriptions();

    //  build pipeline
    GraphicsPipelineBuilder builder;
    builder.setShaders(vertexShader.getShaderModule(), fragmentShader.getShaderModule());
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisamplingNone();
    builder.disableBlending(); //  opaque pipeline
    builder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    std::vector<VkFormat> colorFormats{mRenderer->getSwapChainImageFormat()};
    builder.setColorAttachmentFormats(colorFormats);
    builder.setDepthFormat(mRenderer->getDepthImageFormat());
    builder.setPipelineLayout(mPipelineLayout);

    mPipeline = builder.build(device);

    return BUNNY_HAPPY;
}
} // namespace Bunny::Render
