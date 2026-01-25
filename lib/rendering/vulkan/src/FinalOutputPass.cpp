#include "FinalOutputPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Shader.h"
#include "GraphicsPipelineBuilder.h"
#include "Helper.h"
#include "TextureBank.h"
#include "Error.h"
#include "ErrorCheck.h"

namespace Bunny::Render
{
FinalOutputPass::FinalOutputPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const TextureBank* textureBank, std::string_view vertShader, std::string_view fragShader)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mTextureBank(textureBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void FinalOutputPass::draw() const
{
    VkCommandBuffer cmdBuf = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    //  wait for scene render and cloud render to finish
    //  huh? should probably transition the image layout to shader read only optimal
    //  not sure why I didn't do that
    VkImageMemoryBarrier sceneRenderBarrier = makeImageMemoryBarrier(frame.mRenderedSceneTexture->mImage,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier cloudRenderBarrier =
        makeImageMemoryBarrier(frame.mCloudTexture->mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier fogShadowRenderBarrier =
        makeImageMemoryBarrier(frame.mFogShadowTexture->mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

    VkImageMemoryBarrier barriers[] = {cloudRenderBarrier, fogShadowRenderBarrier};

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);

    VkImageMemoryBarrier sceneBarriers[] = {sceneRenderBarrier};

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, sceneBarriers);

    mRenderer->beginRender(false);

    //  bind pipeline and resources
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    vkCmdBindDescriptorSets(
        cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &frame.mTextureDescSet, 0, nullptr);

    //  bind vertex and index buffers
    VkBuffer vertexBuffers[] = {mVertexBuffer.mBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, mIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT32);

    //  draw
    vkCmdDrawIndexed(cmdBuf, 6, 1, 0, 0, 0);

    mRenderer->finishRender();
}

void FinalOutputPass::updateInputTextures(const AllocatedImage* cloudTexture, const AllocatedImage* fogShadowTexture,
    const AllocatedImage* renderedSceneTexture)
{
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    VkDevice device = mVulkanResources->getDevice();

    frame.mDescriptorAllocator.clearPools(device);
    frame.mCloudTexture = cloudTexture;
    frame.mFogShadowTexture = fogShadowTexture;
    frame.mRenderedSceneTexture = renderedSceneTexture;

    frame.mDescriptorAllocator.allocate(device, &mTextureDescSetLayout, &frame.mTextureDescSet);

    DescriptorWriter writer;
    writer.writeImage(0, frame.mRenderedSceneTexture->mImageView, mTextureBank->getSampler(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(1, frame.mCloudTexture->mImageView, mTextureBank->getSampler(), VK_IMAGE_LAYOUT_GENERAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(2, frame.mFogShadowTexture->mImageView, mTextureBank->getSampler(), VK_IMAGE_LAYOUT_GENERAL,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(device, frame.mTextureDescSet);
}

BunnyResult FinalOutputPass::initPipeline()
{
    //  load shader
    Shader vertexShader(mVertexShaderPath, mVulkanResources->getDevice());
    Shader fragmentShader(mFragmentShaderPath, mVulkanResources->getDevice());

    //  build pipeline layout
    VkDescriptorSetLayout layouts[] = {mTextureDescSetLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout);
    mDeletionStack.AddFunction(
        [this]() { vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr); });

    //  vertex info
    auto bindingDescription = getBindingDescription<ScreenQuadVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = ScreenQuadVertex::getAttributeDescriptions();

    //  build pipeline
    GraphicsPipelineBuilder builder;
    builder.addShaderStage(vertexShader.getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT);
    builder.addShaderStage(fragmentShader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT);
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

BunnyResult FinalOutputPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())
    VkDevice device = mVulkanResources->getDevice();

    for (FrameData& frame : mFrameData)
    {
        DescriptorAllocator::PoolSize poolSizes[] = {
            {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 3},
        };
        frame.mDescriptorAllocator.init(device, 1, poolSizes);

        //  no need to allocate descriptor sets here, they will be allocated every frame
    }

    mDeletionStack.AddFunction([this]() {
        for (FrameData& frame : mFrameData)
        {
            frame.mDescriptorAllocator.destroyPools(mVulkanResources->getDevice());
        }
    });

    return BUNNY_HAPPY;
}

BunnyResult FinalOutputPass::initDataAndResources()
{
    const VkDeviceSize vertexSize = getContainerDataSize(mVertexData);
    const VkDeviceSize indexSize = getContainerDataSize(mIndexData);

    mVulkanResources->createBufferWithData(mVertexData.data(), vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO,
        mVertexBuffer); //  use mapped and sequential bit for easier update via memcpy, might optimize later
    mVulkanResources->createBufferWithData(mIndexData.data(), indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_MEMORY_USAGE_AUTO, mIndexBuffer);

    mDeletionStack.AddFunction([this]() {
        mVulkanResources->destroyBuffer(mIndexBuffer);
        mVulkanResources->destroyBuffer(mVertexBuffer);
    });

    return BUNNY_HAPPY;
}

BunnyResult FinalOutputPass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding imageBinding{
        0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

    DescriptorLayoutBuilder builder;
    builder.addBinding(imageBinding); //  rendered scene
    imageBinding.binding = 1;
    builder.addBinding(imageBinding); //  cloud
    imageBinding.binding = 2;
    builder.addBinding(imageBinding); //  fog shadow
    mTextureDescSetLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction(
        [this]() { vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mTextureDescSetLayout, nullptr); });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
