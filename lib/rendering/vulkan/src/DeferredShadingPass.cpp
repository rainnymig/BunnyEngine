#include "DeferredShadingPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "GraphicsPipelineBuilder.h"
#include "Shader.h"
#include "ShaderData.h"
#include "Helper.h"

namespace Bunny::Render
{
DeferredShadingPass::DeferredShadingPass(
    const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
}

void DeferredShadingPass::initializePass()
{
    initDescriptorSets();
    initPipeline();
    createSampler();
    buildScreenQuad();
}

void DeferredShadingPass::linkLightData(const AllocatedBuffer& lightBuffer)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, lightBuffer.mBuffer, sizeof(LightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (VkDescriptorSet set : mLightDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void DeferredShadingPass::linkGBufferMaps(const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>& colorMaps,
    const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>& fragPosMaps,
    const std::array<AllocatedImage, MAX_FRAMES_IN_FLIGHT>& normalTexCoordMaps)
{
    mColorMaps = &colorMaps;
    mFragPosMaps = &fragPosMaps;
    mNormalTexCoordMaps = &normalTexCoordMaps;

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        DescriptorWriter writer;
        writer.writeImage(0, mColorMaps->at(i).mImageView, mGbufferImageSampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(1, mFragPosMaps->at(i).mImageView, mGbufferImageSampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(2, mNormalTexCoordMaps->at(i).mImageView, mGbufferImageSampler,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.updateSet(mVulkanResources->getDevice(), mGBufferDescSets[i]);
    }
}

void DeferredShadingPass::draw()
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    uint32_t currentFrameIdx = mRenderer->getCurrentFrameIdx();

    mRenderer->beginRender(false);

    //  barrier to make sure rendering to gbuffers is finished
    VkImageMemoryBarrier colorWriteBarrier = makeImageMemoryBarrier(mColorMaps->at(currentFrameIdx).mImage,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier fragPosWriteBarrier = makeImageMemoryBarrier(mFragPosMaps->at(currentFrameIdx).mImage,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier normalTexCoordWriteBarrier =
        makeImageMemoryBarrier(mNormalTexCoordMaps->at(currentFrameIdx).mImage, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);

    VkImageMemoryBarrier barriers[] = {colorWriteBarrier, fragPosWriteBarrier, normalTexCoordWriteBarrier};

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 3, barriers);

    //  bind pipeline and resources
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 1, &mLightDescSets[currentFrameIdx], 0, nullptr);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1, 1, &mGBufferDescSets[currentFrameIdx], 0, nullptr);

    //  bind vertex and index buffers
    VkBuffer vertexBuffers[] = {mVertexBuffer.mBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmd, mIndexBuffer.mBuffer, 0, VK_INDEX_TYPE_UINT32);

    //  draw
    vkCmdDrawIndexed(cmd, 6, 1, 0, 0, 0);

    mRenderer->finishRender();
}

void DeferredShadingPass::cleanup()
{
    VkDevice device = mVulkanResources->getDevice();

    vkDestroyPipeline(device, mPipeline, nullptr);
    vkDestroyPipelineLayout(device, mPipelineLayout, nullptr);

    mDescriptorAllocator.destroyPools(device);

    vkDestroyDescriptorSetLayout(device, mUniformDescLayout, nullptr);
    vkDestroyDescriptorSetLayout(device, mGBufferDescLayout, nullptr);

    vkDestroySampler(device, mGbufferImageSampler, nullptr);

    mVulkanResources->destroyBuffer(mIndexBuffer);
    mVulkanResources->destroyBuffer(mVertexBuffer);
}

void DeferredShadingPass::initDescriptorSets()
{
    VkDevice device = mVulkanResources->getDevice();

    //  build descriptor set layouts
    DescriptorLayoutBuilder layoutBuilder;
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        layoutBuilder.addBinding(uniformBufferLayout);
    }
    mUniformDescLayout = layoutBuilder.build(device);

    layoutBuilder.clear();
    {
        VkDescriptorSetLayoutBinding imageLayout{};
        imageLayout.binding = 0;
        imageLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        imageLayout.descriptorCount = 1;
        imageLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        imageLayout.pImmutableSamplers = nullptr;
        layoutBuilder.addBinding(imageLayout);
        imageLayout.binding = 1;
        layoutBuilder.addBinding(imageLayout);
        imageLayout.binding = 2;
        layoutBuilder.addBinding(imageLayout);
    }
    mGBufferDescLayout = layoutBuilder.build(device);

    //  init descriptor allocator
    //  create descriptor for object buffer and scene buffer
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 2},
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 6}
    };
    mDescriptorAllocator.init(mVulkanResources->getDevice(), 2, poolSizes);

    //  allocator descriptor sets
    for (size_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mUniformDescLayout, &mLightDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mGBufferDescLayout, &mGBufferDescSets[idx], 1, nullptr);
    }
}

void DeferredShadingPass::createSampler()
{
    VkSamplerCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_NEAREST;
    createInfo.minFilter = VK_FILTER_NEAREST;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.minLod = 0;
    createInfo.maxLod = 1;
    createInfo.pNext = nullptr;

    vkCreateSampler(mVulkanResources->getDevice(), &createInfo, 0, &mGbufferImageSampler);
}

BunnyResult DeferredShadingPass::initPipeline()
{
    //  load shader
    Shader vertexShader(mVertexShaderPath, mVulkanResources->getDevice());
    Shader fragmentShader(mFragmentShaderPath, mVulkanResources->getDevice());

    //  build pipeline layout
    VkDescriptorSetLayout layouts[] = {mUniformDescLayout, mGBufferDescLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

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
    builder.disableBlending();                                                //  opaque pipeline
    builder.disableDepthTest();                                               //  no need to do depth test
    std::vector<VkFormat> colorFormats{mRenderer->getSwapChainImageFormat()}; //  color, frag pos, normal texcoord
    builder.setColorAttachmentFormats(colorFormats);
    // builder.setDepthFormat(mRenderer->getDepthImageFormat());
    builder.setPipelineLayout(mPipelineLayout);

    mPipeline = builder.build(mVulkanResources->getDevice());

    return BUNNY_HAPPY;
}

void DeferredShadingPass::buildScreenQuad()
{
    //  fill vertex and index data for screen quad
    mVertexData[0] = ScreenQuadVertex{
        {-1, 1, 0},
        {0, 0}
    }; //  top left
    mVertexData[1] = ScreenQuadVertex{
        {-1, -1, 0},
        {0, 1}
    }; //  bottom left
    mVertexData[2] = ScreenQuadVertex{
        {1, -1, 0},
        {1, 1}
    }; //  bottom right
    mVertexData[3] = ScreenQuadVertex{
        {1, 1, 0},
        {1, 0}
    }; //  top right

    mIndexData = std::array<uint32_t, 6>{0, 1, 3, 3, 1, 2};

    const VkDeviceSize vertexSize = mVertexData.size() * sizeof(ScreenQuadVertex);
    const VkDeviceSize indexSize = mIndexData.size() * sizeof(uint32_t);

    mVulkanResources->createAndMapBuffer(mVertexData.data(), vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mVertexBuffer);
    mVulkanResources->createAndMapBuffer(mIndexData.data(), indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT, VMA_MEMORY_USAGE_GPU_ONLY, mIndexBuffer);
}

} // namespace Bunny::Render
