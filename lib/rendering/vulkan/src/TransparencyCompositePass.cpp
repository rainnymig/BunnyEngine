#include "TransparencyCompositePass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Error.h"
#include "ErrorCheck.h"
#include "GraphicsPipelineBuilder.h"

namespace Bunny::Render
{
TransparencyCompositePass::TransparencyCompositePass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
    std::string_view vertShader, std::string_view fragShader)
    : super(vulkanResources, renderer, materialBank, meshBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void TransparencyCompositePass::draw() const
{
    VkCommandBuffer cmdBuf = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    {
        VkImageMemoryBarrier accumRenderBarrier = makeImageMemoryBarrier(frame.mAccumulateImage->mImage,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier revealRenderBarrier = makeImageMemoryBarrier(frame.mRevealageImage->mImage,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier sceneRenderBarrier = makeImageMemoryBarrier(frame.mSceneRenderTarget->mImage,
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

        //  barrier for transparent images
        VkImageMemoryBarrier barriers[] = {accumRenderBarrier, revealRenderBarrier};
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);

        //  barrier for render target
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &sceneRenderBarrier);
    }

    auto renderHelper = mRenderer->getRenderHelper()
                            .addColorAttachment(frame.mSceneRenderTarget->mImageView)
                            .setDepthTest(false)
                            .beginRender();

    //  bind pipeline and resources
    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    vkCmdBindDescriptorSets(
        cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &frame.mTransparentImageDescSet, 0, nullptr);

    //  bind vertex and index buffers
    VkBuffer vertexBuffers[] = {mVertexBuffer.mBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, mIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT32);

    //  draw
    vkCmdDrawIndexed(cmdBuf, 6, 1, 0, 0, 0);

    renderHelper.finishRender();
}

void TransparencyCompositePass::setSceneRenderTarget(const AllocatedImage* sceneRenderTarget)
{
    mFrameData[mRenderer->getCurrentFrameIdx()].mSceneRenderTarget = sceneRenderTarget;
}

void TransparencyCompositePass::linkTransparentImages(
    const std::array<const AllocatedImage*, MAX_FRAMES_IN_FLIGHT>& accumImages,
    const std::array<const AllocatedImage*, MAX_FRAMES_IN_FLIGHT>& revealImages)
{
    DescriptorWriter writer;
    for (int idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        FrameData& frame = mFrameData[idx];
        frame.mAccumulateImage = accumImages[idx];
        frame.mRevealageImage = revealImages[idx];

        writer.clear();
        writer.writeImage(0, accumImages[idx]->mImageView, mImageSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(1, revealImages[idx]->mImageView, mImageSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.updateSet(mVulkanResources->getDevice(), frame.mTransparentImageDescSet);
    }
}

BunnyResult TransparencyCompositePass::initPipeline()
{
    VkDevice device = mVulkanResources->getDevice();

    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initPipelineLayout())

    //  load shader file
    Shader vertexShader(mVertexShaderPath, device);
    Shader fragmentShader(mFragmentShaderPath, device);

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
    builder.addColorAttachmentWithBlend(mRenderer->getSwapChainImageFormat(),
        makePipelineColorBlendAttachmentState(VK_BLEND_FACTOR_SRC_ALPHA, VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA));
    builder.disableDepthTest();
    builder.setDepthFormat(mRenderer->getDepthImageFormat());
    builder.setPipelineLayout(mPipelineLayout);

    mPipeline = builder.build(device);

    mDeletionStack.AddFunction([this]() { vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult TransparencyCompositePass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())

    VkDevice device = mVulkanResources->getDevice();
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 2},
    };
    mDescriptorAllocator.init(device, 2, poolSizes);

    for (FrameData& frame : mFrameData)
    {
        mDescriptorAllocator.allocate(device, &mImagesDescLayout, &frame.mTransparentImageDescSet, 1);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult TransparencyCompositePass::initDataAndResources()
{
    //  create vertex and index buffers
    //  should use a universal v and i buffer for all screen quads
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

    //  create sampler
    VkSamplerCreateInfo createInfo = {};

    //  check filter and mipmap mode later
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.minLod = 0;
    createInfo.maxLod = 0;
    createInfo.pNext = nullptr;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateSampler(mVulkanResources->getDevice(), &createInfo, 0, &mImageSampler))

    mDeletionStack.AddFunction([this]() { vkDestroySampler(mVulkanResources->getDevice(), mImageSampler, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult TransparencyCompositePass::initPipelineLayout()
{
    //  build pipeline layout
    VkDescriptorSetLayout layouts[] = {mImagesDescLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout);
    mDeletionStack.AddFunction(
        [this]() { vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult TransparencyCompositePass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding imageBinding{
        0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};

    DescriptorLayoutBuilder builder;
    builder.addBinding(imageBinding); //  accumulation
    imageBinding.binding = 1;
    builder.addBinding(imageBinding); //  revealage
    mImagesDescLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction(
        [this]() { vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mImagesDescLayout, nullptr); });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
