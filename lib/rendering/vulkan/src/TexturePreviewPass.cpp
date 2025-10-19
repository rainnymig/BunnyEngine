#include "TexturePreviewPass.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "MaterialBank.h"
#include "TextureBank.h"
#include "GraphicsPipelineBuilder.h"
#include "Shader.h"
#include "Vertex.h"

#include <imgui.h>

namespace Bunny::Render
{

TexturePreviewPass::TexturePreviewPass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
    TextureBank* textureBank, std::string_view vertShader, std::string_view fragShader)
    : super(vulkanResources, renderer, materialBank, meshBank),
      mTextureBank(textureBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void TexturePreviewPass::draw() const
{
    if (!mIsActive)
    {
        return;
    }
}

void TexturePreviewPass::showImguiControls()
{
    ImGui::Begin("Texture Preview");

    ImGui::Checkbox("Show texture preview", &mIsActive);
    ImGui::Separator();

    if (mIsActive)
    {
    }

    ImGui::End();
}

void TexturePreviewPass::setIsActive(bool isActive)
{
    mIsActive = isActive;
}

BunnyResult TexturePreviewPass::initPipeline()
{
    //  load shader
    Shader vertexShader(mVertexShaderPath, mVulkanResources->getDevice());
    Shader fragmentShader(mFragmentShaderPath, mVulkanResources->getDevice());

    //  build pipeline layout
    VkDescriptorSetLayout layouts[] = {mTextureDescSetLayout};

    VkPushConstantRange pushConstRange{
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(PreviewParams)};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;

    vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout);

    //  vertex info
    auto bindingDescription = getBindingDescription<ScreenQuadVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = ScreenQuadVertex::getAttributeDescriptions();

    //  build pipeline
    GraphicsPipelineBuilder builder;
    builder.setShaders(vertexShader.getShaderModule(), fragmentShader.getShaderModule());
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisamplingNone();
    builder.disableBlending();  //  opaque pipeline
    builder.disableDepthTest(); //  no need to do depth test
    std::vector<VkFormat> colorFormats{mRenderer->getSwapChainImageFormat()};
    builder.setColorAttachmentFormats(colorFormats);
    // builder.setDepthFormat(mRenderer->getDepthImageFormat());
    builder.setPipelineLayout(mPipelineLayout);

    mPipeline = builder.build(mVulkanResources->getDevice());

    return BUNNY_HAPPY;
}

BunnyResult TexturePreviewPass::initDescriptors()
{
    return BunnyResult();
}

BunnyResult TexturePreviewPass::initDataAndResources()
{
    return BunnyResult();
}
} // namespace Bunny::Render
