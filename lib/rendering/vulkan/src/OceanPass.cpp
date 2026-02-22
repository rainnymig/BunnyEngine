#include "OceanPass.h"

#include "Descriptor.h"
#include "ErrorCheck.h"
#include "Error.h"
#include "Shader.h"
#include "GraphicsPipelineBuilder.h"
#include "TextureBank.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Helper.h"

#include <glm/gtx/euler_angles.hpp>

namespace Bunny::Render
{
OceanPass::OceanPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const TextureBank* textureBank, float waveAreaWidth, uint32_t patternAreaGridCount, uint32_t totalGridCount,
    std::string_view meshShaderPath, std::string_view fragShaderPath)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mMeshShaderPath(meshShaderPath),
      mFragShaderPath(fragShaderPath),
      mTextureBank(textureBank),
      mTotalWaveGridCount(totalGridCount)
{
    //  create wave field and world param data
    mWaveParams.mGridAreaWidth = waveAreaWidth;
    mWaveParams.mGridCellWidth = waveAreaWidth / patternAreaGridCount;
    mWaveParams.mGridOrigin = glm::vec2(-mWaveParams.mGridCellWidth * mTotalWaveGridCount / 2);
}

void OceanPass::draw() const
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    VkImageMemoryBarrier waveDisplacementImageBarrier = makeImageMemoryBarrier(frame.mVertexDisplacementImage->mImage,
        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier waveNormalImageBarrier =
        makeImageMemoryBarrier(frame.mVertexNormalImage->mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier barriers[]{waveDisplacementImageBarrier, waveNormalImageBarrier};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_MESH_SHADER_BIT_EXT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);

    //  wait for rendering from other passes onto the render target is done
    VkImageMemoryBarrier renderTargetBarrier = makeImageMemoryBarrier(frame.mRenderTarget->mImage,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &renderTargetBarrier);

    //  render
    std::vector<VkImageView> colorAttachmentViews;
    colorAttachmentViews.push_back(frame.mRenderTarget->mImageView);
    static constexpr bool updateDepth = true;
    static constexpr bool clearColor = false;
    mRenderer->beginRender(colorAttachmentViews, updateDepth, clearColor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 3, &frame.mMeshDescSet, 0, nullptr);

    //  dispatch mesh pipeline
    vkCmdDrawMeshTasksEXT(cmd, mTotalWaveGridCount / MESH_THREAD_COUNT_X, mTotalWaveGridCount / MESH_THREAD_COUNT_Y, 1);

    mRenderer->finishRender();
}

void OceanPass::prepareFrameDescriptors()
{
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    VkDevice device = mVulkanResources->getDevice();
    frame.mDescriptorAllocator.clearPools(device);

    //  allocate discriptors for this frame
    //  mainly because we need to bind the updated wave textures
    //  maybe find a solution later to avoid reallocating descriptors every frame
    //  because the wave textures for each frame is basically fixed and can be bound upfront
    //  this is just an easy and lazy temp solution

    //  allocate descriptor sets
    VkDescriptorSetLayout descLayouts[] = {mMeshDescLayout, mWaveImageDescLayout, mFragDescLayout};
    frame.mDescriptorAllocator.allocate(device, descLayouts, &frame.mMeshDescSet, 3);

    //  write the resources to the desc sets
    DescriptorWriter writer;
    writer.writeBuffer(0, mWaveParamsBuffer.mBuffer, sizeof(WaveFieldParams), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeBuffer(1, mWorldParamsBuffer.mBuffer, sizeof(WorldParams), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.updateSet(device, frame.mMeshDescSet);

    writer.clear();
    writer.writeImage(0, frame.mVertexDisplacementImage->mImageView, mTextureBank->getSampler(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.writeImage(1, frame.mVertexNormalImage->mImageView, mTextureBank->getSampler(),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    writer.updateSet(device, frame.mWaveImageDescSet);

    writer.clear();
    writer.writeBuffer(0, mLightDataBuffer->mBuffer, sizeof(LightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeBuffer(1, mCameraDataBuffer->mBuffer, sizeof(PbrCameraData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.updateSet(device, frame.mFragDescSet);
}

void OceanPass::updateWorldParams(const glm::mat4& mvpMatrix, float elapsedTime, float deltaTime)
{
    mWorldParams.mMvpMatrix = mvpMatrix;
    mWorldParams.mElapsedTime = elapsedTime;
    mWorldParams.mDeltaTime = deltaTime;

    void* data = mWorldParamsBuffer.mAllocationInfo.pMappedData;
    memcpy(data, &mWorldParams, sizeof(WorldParams));
}

void OceanPass::linkLightAndCameraData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData)
{
    mLightDataBuffer = &lightData;
    mCameraDataBuffer = &cameraData;
}

void OceanPass::updateRenderTarget(const AllocatedImage* renderTarget)
{
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    frame.mRenderTarget = renderTarget;
}

void OceanPass::updateWaveTextures(const AllocatedImage* vertexDisplacementTex, const AllocatedImage* vertexNormalTex)
{
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    frame.mVertexDisplacementImage = vertexDisplacementTex;
    frame.mVertexNormalImage = vertexNormalTex;
}

BunnyResult OceanPass::initPipeline()
{
    VkDevice device = mVulkanResources->getDevice();

    Shader meshShader(mMeshShaderPath, device);
    Shader fragShader(mFragShaderPath, device);

    VkDescriptorSetLayout layouts[] = {mMeshDescLayout, mWaveImageDescLayout, mFragDescLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 3;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout))
    mDeletionStack.AddFunction(
        [this]() { vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr); });

    GraphicsPipelineBuilder pipelineBuilder;
    pipelineBuilder.addShaderStage(meshShader.getShaderModule(), VK_SHADER_STAGE_MESH_BIT_EXT);
    pipelineBuilder.addShaderStage(fragShader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending(); //  opaque pipeline
    pipelineBuilder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    std::vector<VkFormat> colorFormats{mRenderer->getSwapChainImageFormat()};
    pipelineBuilder.setColorAttachmentFormats(colorFormats);
    pipelineBuilder.setDepthFormat(mRenderer->getDepthImageFormat());
    pipelineBuilder.setPipelineLayout(mPipelineLayout);
    mPipeline = pipelineBuilder.build(device);
    if (mPipeline == nullptr)
    {
        return BUNNY_SAD;
    }

    mDeletionStack.AddFunction([this]() { vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult OceanPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 4},
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 2},
    };
    for (FrameData& frame : mFrameData)
    {
        frame.mDescriptorAllocator.init(device, 4, poolSizes);
    }

    //  descriptors will be allocated every frame when wave textures are updated

    mDeletionStack.AddFunction([this]() {
        for (FrameData& frame : mFrameData)
        {
            frame.mDescriptorAllocator.destroyPools(mVulkanResources->getDevice());
        }
    });

    return BUNNY_HAPPY;
}

BunnyResult OceanPass::initDataAndResources()
{
    //  create wave field and world buffers
    mVulkanResources->createBufferWithData(&mWaveParams, sizeof(WaveFieldParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO, mWaveParamsBuffer);
    mWorldParamsBuffer = mVulkanResources->createBuffer(sizeof(WorldParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    mDeletionStack.AddFunction([this]() {
        mVulkanResources->destroyBuffer(mWaveParamsBuffer);
        mVulkanResources->destroyBuffer(mWorldParamsBuffer);
    });

    return BUNNY_HAPPY;
}

BunnyResult OceanPass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding descBinding{
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_MESH_BIT_EXT, nullptr};

    VkDevice device = mVulkanResources->getDevice();

    DescriptorLayoutBuilder builder;
    builder.addBinding(descBinding); //  wave field params
    descBinding.binding = 1;
    builder.addBinding(descBinding); //  world params
    mMeshDescLayout = builder.build(device);

    builder.clear();
    descBinding.binding = 0;
    descBinding.stageFlags = VK_SHADER_STAGE_MESH_BIT_EXT;
    descBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    builder.addBinding(descBinding); //  wave vertex displacement texture
    descBinding.binding = 1;
    builder.addBinding(descBinding); //  wave vertex normal texture
    mWaveImageDescLayout = builder.build(device);

    builder.clear();
    descBinding.binding = 0;
    descBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    descBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    builder.addBinding(descBinding); //  light data
    descBinding.binding = 1;
    builder.addBinding(descBinding); //  camera data
    mFragDescLayout = builder.build(device);

    mDeletionStack.AddFunction([this]() {
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mMeshDescLayout, nullptr);
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mWaveImageDescLayout, nullptr);
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mFragDescLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
