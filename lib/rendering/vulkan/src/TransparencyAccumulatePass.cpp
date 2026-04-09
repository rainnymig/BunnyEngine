#include "TransparencyAccumulatePass.h"

#include "MaterialBank.h"
#include "Vertex.h"
#include "GraphicsPipelineBuilder.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Helper.h"

namespace Bunny::Render
{
Render::TransparencyAccumulatePass::TransparencyAccumulatePass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
    std::string_view vertShader, std::string_view fragShader)
    : super(vulkanResources, renderer, materialBank, meshBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void Render::TransparencyAccumulatePass::draw() const
{
    //  only draw opaque surfaces, skip if none
    if (mMeshBank->getTransparentSurfaceCount() == 0)
    {
        return;
    }

    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    //  transition the render target image layout back to color attachment optimal
    //  no need to transition the multi sampled image, because they are always attachment optimal
    VkImageMemoryBarrier accuBarrier = makeImageMemoryBarrier(frame.mAccumulateImage.mImage, VK_ACCESS_SHADER_READ_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier revealBarrier = makeImageMemoryBarrier(frame.mRevealageImage.mImage, VK_ACCESS_SHADER_READ_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_COLOR_BIT);
    {
        VkImageMemoryBarrier barriers[] = {accuBarrier, revealBarrier};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);
    }

    bool multiSample = mRenderer->isMultiSampleEnabled();
    auto renderHelper = mRenderer->getRenderHelper().setClearDepth(false).setDepthTest(true).setDepthMultiSample(
        mRenderer->isMultiSampleEnabled(), false);
    VkClearValue accuClear{
        .color{0, 0, 0, 0}
    };
    VkClearValue revealClear{
        .color{1, 0, 0, 0}
    };
    if (multiSample)
    {
        //  if multi sample,
        renderHelper.addColorAttachment(
            frame.mAccumulateImageMultiSampled.mImageView, true, accuClear, frame.mAccumulateImage.mImageView);
        renderHelper.addColorAttachment(
            frame.mRevealageImageMultiSampled.mImageView, true, revealClear, frame.mRevealageImage.mImageView);
    }
    else
    {
        renderHelper.addColorAttachment(frame.mAccumulateImage.mImageView, true, accuClear);
        renderHelper.addColorAttachment(frame.mRevealageImage.mImageView, true, revealClear);
    }
    renderHelper.beginRender();

    //  bind mesh vertex and index buffers
    mMeshBank->bindMeshBuffers(cmd);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    //  bind all scene, object and material descriptor sets at once
    //  they are laid out properly in FrameData
    //  if the layout changes this needs to be updated
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 4,
        &mFrameData[mRenderer->getCurrentFrameIdx()].mWorldDescSet, 0, nullptr);

    //  only draw the transparent surfaces
    vkCmdDrawIndexedIndirect(cmd, mDrawCommandsBuffer->mBuffer, mMeshBank->getOpaqueSurfaceCount(),
        mMeshBank->getTransparentSurfaceCount(), sizeof(VkDrawIndexedIndirectCommand));

    renderHelper.finishRender();
}

void Render::TransparencyAccumulatePass::linkWorldData(
    const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, lightData.mBuffer, sizeof(PbrLightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeBuffer(1, cameraData.mBuffer, sizeof(PbrCameraData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (const FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mWorldDescSet);
    }
}

void Render::TransparencyAccumulatePass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
    Render::DescriptorWriter writer;
    writer.writeBuffer(0, objectBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (const FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mObjectDescSet);
    }
}

void Render::TransparencyAccumulatePass::linkShadowData(std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> shadowImageViews)
{
    Render::DescriptorWriter writer;
    VkDevice device = mVulkanResources->getDevice();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        const FrameData& frame = mFrameData[i];
        writer.clear();
        writer.writeImage(0, shadowImageViews[i], nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, frame.mEffectDescSet);
    }
}

void TransparencyAccumulatePass::setDrawCommandsBuffer(const AllocatedBuffer& buffer)
{
    mDrawCommandsBuffer = &buffer;
}

BunnyResult Render::TransparencyAccumulatePass::initPipeline()
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
    builder.addShaderStage(vertexShader.getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT);
    builder.addShaderStage(fragmentShader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT);
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    if (mRenderer->isMultiSampleEnabled())
    {
        builder.setMultiSamplingCount(mRenderer->getRenderMultiSampleCount());
    }
    else
    {
        builder.setMultisamplingNone();
    }

    builder.addColorAttachmentWithBlend(VK_FORMAT_R32G32B32A32_SFLOAT,
        makePipelineColorBlendAttachmentState(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE)); //  accumulate
    builder.addColorAttachmentWithBlend(VK_FORMAT_R16_SFLOAT, //  maybe R8_UNORM or R8_SNORM is enough?
        makePipelineColorBlendAttachmentState(VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR)); //  revealage

    //  depth test is enabled so that we don't render transparent objects behind opaque ones
    //  but depth write is disabled so that transparent objects don't block each other
    builder.enableDepthTest(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.setDepthFormat(mRenderer->getDepthImageFormat());
    builder.setPipelineLayout(mPipelineLayout);

    mPipeline = builder.build(device);

    mDeletionStack.AddFunction([this]() { vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult Render::TransparencyAccumulatePass::initDescriptors()
{
    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .mRatio = 4                                  },
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 4                                  },
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = PbrMaterialBank::TEXTURE_ARRAY_SIZE},
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          .mRatio = 2                                  },
    };
    mDescriptorAllocator.init(device, 10, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mMaterialBank->getWorldDescSetLayout(),
        mMaterialBank->getObjectDescSetLayout(), mMaterialBank->getMaterialDescSetLayout(),
        mMaterialBank->getEffectDescSetLayout()};
    for (FrameData& frame : mFrameData)
    {
        //  allocate all 4 sets of one frame at once
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mWorldDescSet, 4);

        //  link material data to material descriptor set
        mMaterialBank->updateMaterialDescriptorSet(frame.mMaterialDescSet, mMeshBank);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });
    return BUNNY_HAPPY;
}

BunnyResult Render::TransparencyAccumulatePass::initDataAndResources()
{
    VkExtent3D renderTargetExtent{mRenderer->getSwapChainExtent().width, mRenderer->getSwapChainExtent().height, 1};
    //  create the render targets
    for (FrameData& frame : mFrameData)
    {
        //  accumulate
        frame.mAccumulateImageMultiSampled =
            mVulkanResources->createImage(renderTargetExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false,
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, mRenderer->getRenderMultiSampleCount());
        frame.mAccumulateImage = mVulkanResources->createImage(renderTargetExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        //  revealage
        frame.mRevealageImage = mVulkanResources->createImage(renderTargetExtent, VK_FORMAT_R16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, mRenderer->getRenderMultiSampleCount());
        frame.mRevealageImage = mVulkanResources->createImage(renderTargetExtent, VK_FORMAT_R16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    mDeletionStack.AddFunction([this]() {
        for (FrameData& frame : mFrameData)
        {
            mVulkanResources->destroyImage(frame.mAccumulateImage);
            mVulkanResources->destroyImage(frame.mRevealageImage);
        }
    });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
