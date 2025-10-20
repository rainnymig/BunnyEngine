#include "TexturePreviewPass.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "MaterialBank.h"
#include "TextureBank.h"
#include "GraphicsPipelineBuilder.h"
#include "Shader.h"
#include "Vertex.h"
#include "Error.h"

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
        const std::vector<AllocatedImage>& textures = mTextureBank->getAllTextures();
        if (!textures.empty())
        {
        }
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
    mDeletionStack.AddFunction(
        [this]() { vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr); });

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
    mDeletionStack.AddFunction([this]() { vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult TexturePreviewPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 2},
    };
    mDescriptorAllocator.init(device, 2, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mTextureDescSetLayout};
    for (FrameData& frame : mFrameData)
    {
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mTextureDescSet, 1);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult TexturePreviewPass::initDataAndResources()
{
    //  build vertex buffer for screen quad

    return BUNNY_HAPPY;
}

bool TexturePreviewPass::shouldUpdatePreviewTexture() const
{
    return mPreviewParams.mIs3d ? mTex3dIdPreviewing != mTex3dIdToPreview : mTex2dIdPreviewing != mTex2dIdToPreview;
}

void TexturePreviewPass::updateTextureForPreview()
{
    //  wait for current rendering to end?

    if (shouldUpdatePreviewTexture())
    {
        mTexturePreviewReady = false;

        AllocatedImage texToPreview;
        IdType texIdToPreview = mPreviewParams.mIs3d ? mTex3dIdToPreview : mTex2dIdToPreview;
        if (!mTextureBank->getTexture(texIdToPreview, texToPreview))
        {
            return;
        }

        //  if the texture selected to be previewed is not the one being previewed
        //  recreate the descriptor sets to use the new texture
        VkDevice device = mVulkanResources->getDevice();
        mDescriptorAllocator.clearPools(device);

        VkDescriptorSetLayout descLayouts[] = {mTextureDescSetLayout};
        for (FrameData& frame : mFrameData)
        {
            mDescriptorAllocator.allocate(device, descLayouts, &frame.mTextureDescSet, 1);
        }

        DescriptorWriter writer;
        constexpr uint32_t tex2dBinding = 0;
        constexpr uint32_t tex3dBinding = 1;
        writer.writeImage(mPreviewParams.mIs3d ? tex3dBinding : tex3dBinding, texToPreview.mImageView,
            mTextureBank->getSampler(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        for (FrameData& frame : mFrameData)
        {
            writer.updateSet(device, frame.mTextureDescSet);
        }

        if (mPreviewParams.mIs3d)
        {
            mTex3dIdPreviewing = texIdToPreview;
        }
        else
        {
            mTex3dIdPreviewing = texIdToPreview;
        }

        //  update vertex buffer to fit aspect ratio?

        mTexturePreviewReady = true;
    }
}

BunnyResult TexturePreviewPass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding imageBinding{
        0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

    DescriptorLayoutBuilder builder;
    builder.addBinding(imageBinding); //  tex 2D
    imageBinding.binding = 1;
    builder.addBinding(imageBinding); //  tex 3D

    mTextureDescSetLayout = builder.build(mVulkanResources->getDevice());
    mDeletionStack.AddFunction(
        [this]() { vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mTextureDescSetLayout, nullptr); });

    return BUNNY_HAPPY;
}
} // namespace Bunny::Render
