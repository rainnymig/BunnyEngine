#include "SkyPass.h"

#include "Error.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "TextureBank.h"
#include "Shader.h"
#include "Camera.h"
#include "ErrorCheck.h"
#include "ComputePipelineBuilder.h"

#include <cassert>

namespace Bunny::Render
{
static constexpr SkyPass::CloudData defaultCloudData{
    //  .mCloudNoiseDimension
    .mCloudCoverage = 0.995f,
    //  .mDetailNoiseDimension
    .mLightMarchStepSize = 50.0f,
    //  .mRenderResolution
    .mCloudRegionMinXZ = glm::vec2(-10240, -10240),
    .mCloudRegionMaxXZ = glm::vec2(10240, 10240),
    .mG1 = 0.75f,
    .mG2 = -0.2f,
    .mExtinctionCoef = 1.8f,
    .mScatteringCoef = 1.79f,
    .mDensityModifier = 0.2f,
    .mMinRayMarchStepSize = 60.0f,
    // .mZNear
    // .mZFar
    .mDetailErodeModCoef = 0.15f,
    .mLodStartDistance = 6000.0f,
    .mLodStageLength = 3000.0f,
    .mCloudRegionR1 = 130050,
    .mCloudRegionR2 = 132050,
    .mCloudRegionCy = 129250,
};

SkyPass::SkyPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    TextureBank* textureBank, std::string_view cloudShaderPath)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mTextureBank(textureBank),
      mCloudShaderPath(cloudShaderPath)
{
}

void SkyPass::draw() const
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    uint32_t currentFrameIdx = mRenderer->getCurrentFrameIdx();
    const FrameData& frame = mFrameData.at(currentFrameIdx);

    //  should wait for cloud texture read by final output pass is finished
    //  but probably not needed
    //  check back later

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 2, frame.getCurrentDescSets(), 0, nullptr);

    constexpr static uint32_t computeSizeX = 32;
    constexpr static uint32_t computeSizeY = 16;
    VkExtent3D renderImageExtent = frame.mCloudTexture1.mExtent;
    vkCmdDispatch(cmd, renderImageExtent.width / computeSizeX, renderImageExtent.height / computeSizeY, 1);

    //  transition the depth image back to depth attachment optimal
    //  maybe not needed here, because depth reduce pass still need to read
    //  check back later
}

void SkyPass::updateFrameData()
{
    //  advance frame in sequence sothat it will use the correct current frame and last frame data
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    frame.advanceFrameInSequence();

    VkCommandBuffer cmdBuf = mRenderer->getCurrentCommandBuffer();

    //  transition depth image from depth attachment optimal to shader read optimal
    {
        VkImageMemoryBarrier depthWriteFinishBarrier =
            makeImageMemoryBarrier(frame.mDepthTexture->mImage, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
        vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &depthWriteFinishBarrier);
    }

    //  wait for the prev frame cloud texture write finish (maybe not needed because it should be done by this point?)
}

void SkyPass::updateRenderParams(const Camera& camera, float elapsedTime)
{
    //  update camera and time information
    mCloudRenderParams.mPrevViewProj = camera.getPrevViewProjMatrix();
    mCloudRenderParams.mCameraPosition = camera.getPosition();

    constexpr static float frameToCamera = 1.0f;
    glm::vec3 frameCenter = camera.getPosition() + camera.getForward() * frameToCamera;

    float heightToDist = glm::tan(camera.getFov() / 2);
    float frameHalfHeight = heightToDist * frameToCamera;
    float frameHalfWidth = frameHalfHeight * camera.getAspectRatio();

    glm::vec3 camUp = camera.getUp();
    glm::vec3 camRight = camera.getRight();
    mCloudRenderParams.mCameraFrameWorldTL =
        glm::vec4(frameCenter + camUp * frameHalfHeight - camRight * frameHalfWidth, 1.0f);
    mCloudRenderParams.mCameraFrameWorldTR =
        glm::vec4(frameCenter + camUp * frameHalfHeight + camRight * frameHalfWidth, 1.0f);
    mCloudRenderParams.mCameraFrameWorldBL =
        glm::vec4(frameCenter - camUp * frameHalfHeight - camRight * frameHalfWidth, 1.0f);
    mCloudRenderParams.mCameraFrameWorldBR =
        glm::vec4(frameCenter - camUp * frameHalfHeight + camRight * frameHalfWidth, 1.0f);

    mCloudRenderParams.mDitherIdx = (mCloudRenderParams.mDitherIdx + 1) % CloudRenderParams::MAX_DITHER_COUNT;

    mCloudRenderParams.mElapsedTime = elapsedTime;

    //  use the new data to update the render params buffer
    {
        void* data = mCloudRenderParamsBuffer.mAllocationInfo.pMappedData;
        memcpy(data, &mCloudRenderParams, sizeof(CloudRenderParams));
    }
}

void SkyPass::linkLightData(const AllocatedBuffer& lightData)
{
    VkDevice device = mVulkanResources->getDevice();
    DescriptorWriter writer;
    writer.writeBuffer(0, lightData.mBuffer, sizeof(LightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (FrameData& frame : mFrameData)
    {
        for (FrameData::DescSets& descSets : frame.mDescriptors)
        {
            writer.updateSet(device, descSets.mCloudDescSet);
        }
    }
}

const AllocatedImage& SkyPass::getCurrentCloudTexture() const
{
    return mFrameData[mRenderer->getCurrentFrameIdx()].getCloudTexture();
}

const AllocatedImage& SkyPass::getCurrentFogShadowTexture() const
{
    return mFrameData[mRenderer->getCurrentFrameIdx()].mFogShadowTexture;
}

BunnyResult SkyPass::initPipeline()
{
    VkDevice device = mVulkanResources->getDevice();

    //  load shader
    Shader cloudShader(mCloudShaderPath, device);

    //  build pipeline layout
    VkDescriptorSetLayout layouts[] = {mCloudDescSetLayout, mTextureDescSetLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout))
    mDeletionStack.AddFunction(
        [this]() { vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr); });

    ComputePipelineBuilder pipelineBuilder;
    pipelineBuilder.setShader(cloudShader.getShaderModule());
    pipelineBuilder.setPipelineLayout(mPipelineLayout);
    mPipeline = pipelineBuilder.build(device);

    mDeletionStack.AddFunction([this]() { vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult SkyPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts());

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 3},
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 2},
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          .mRatio = 2},
    };
    mDescriptorAllocator.init(device, 8, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mCloudDescSetLayout, mTextureDescSetLayout};
    for (FrameData& frame : mFrameData)
    {
        for (FrameData::DescSets& frameDescSets : frame.mDescriptors)
        {
            mDescriptorAllocator.allocate(device, descLayouts, &frameDescSets.mCloudDescSet, 2);
        }
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult SkyPass::initDataAndResources()
{
    VkExtent2D swapchainExtent = mRenderer->getSwapChainExtent();
    VkExtent3D renderImageExtent{swapchainExtent.width, swapchainExtent.height, 1};
    VkDevice device = mVulkanResources->getDevice();

    //  create output textures
    for (uint32_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        FrameData& frame = mFrameData[idx];
        frame.mCloudTexture1 = mVulkanResources->createImage(renderImageExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false,
            VK_IMAGE_LAYOUT_GENERAL);
        frame.mCloudTexture2 = mVulkanResources->createImage(renderImageExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false,
            VK_IMAGE_LAYOUT_GENERAL);
        frame.mFogShadowTexture = mVulkanResources->createImage(renderImageExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false,
            VK_IMAGE_LAYOUT_GENERAL);
        frame.mDepthTexture = &mRenderer->getDepthImageResolved(idx);
    }

    mDeletionStack.AddFunction([this]() {
        for (FrameData& frame : mFrameData)
        {
            mVulkanResources->destroyImage(frame.mCloudTexture1);
            mVulkanResources->destroyImage(frame.mCloudTexture2);
            mVulkanResources->destroyImage(frame.mFogShadowTexture);
            frame.mDepthTexture = nullptr;
        }
    });

    //  load noise and other textures to texture bank
    IdType texId;
    //  main cloud noise
    if (BUNNY_SUCCESS(mTextureBank->addTexture3d("./assets/texture/main_cloud_noise.png", VK_FORMAT_R8G8B8A8_UNORM,
            MAIN_CLOUD_NOISE_RESOLUTION, MAIN_CLOUD_NOISE_RESOLUTION, MAIN_CLOUD_NOISE_RESOLUTION, texId)))
    {
        mTextureBank->getTexture3d(texId, mMainNoiseTexture);
    }
    //  detail cloud noise
    if (BUNNY_SUCCESS(mTextureBank->addTexture3d("./assets/texture/detail_cloud_noise.png", VK_FORMAT_R8G8B8A8_UNORM,
            DETAIL_CLOUD_NOISE_RESOLUTION, DETAIL_CLOUD_NOISE_RESOLUTION, DETAIL_CLOUD_NOISE_RESOLUTION, texId)))
    {
        mTextureBank->getTexture3d(texId, mDetailNoiseTexture);
    }
    //  weather
    if (BUNNY_SUCCESS(mTextureBank->addTexture("./assets/texture/weather.png", VK_FORMAT_R8G8B8A8_UNORM, texId)))
    {
        mTextureBank->getTexture(texId, mWeatherNoiseTexture);
    }
    //  blue noise
    if (BUNNY_SUCCESS(mTextureBank->addTexture("./assets/texture/blue_noise.png", VK_FORMAT_R8G8B8A8_UNORM, texId)))
    {
        mTextureBank->getTexture(texId, mBlueNoiseTexture);
    }

    //  init and create cloud and render parameter buffers
    //  init data...
    //  init cloud data
    mCloudData = defaultCloudData;
    mCloudData.mCloudNoiseDimension =
        glm::vec3(mMainNoiseTexture.mExtent.width, mMainNoiseTexture.mExtent.height, mMainNoiseTexture.mExtent.depth);
    mCloudData.mDetailNoiseDimension = glm::vec3(
        mDetailNoiseTexture.mExtent.width, mDetailNoiseTexture.mExtent.height, mDetailNoiseTexture.mExtent.depth);
    mCloudData.mRenderResolution = glm::vec2(swapchainExtent.width, swapchainExtent.height);
    mCloudData.mZNear = Camera::NearPlaneDistance;
    mCloudData.mZFar = Camera::FarPlaneDistance;
    //  cloud render params will be updated from camera every frame
    //  create buffers
    mVulkanResources->createBufferWithData(&mCloudData, sizeof(CloudData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO, mCloudDataBuffer);
    mCloudRenderParamsBuffer =
        mVulkanResources->createBuffer(sizeof(CloudRenderParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            VMA_MEMORY_USAGE_AUTO);

    mDeletionStack.AddFunction([this]() {
        mVulkanResources->destroyBuffer(mCloudDataBuffer);
        mVulkanResources->destroyBuffer(mCloudRenderParamsBuffer);
    });

    //  use the created resources to update the frame descriptor sets
    for (FrameData& frame : mFrameData)
    {
        DescriptorWriter writer;
        //  light buffer is linked separately
        writer.writeBuffer(1, mCloudDataBuffer.mBuffer, sizeof(CloudData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.writeBuffer(
            2, mCloudRenderParamsBuffer.mBuffer, sizeof(CloudRenderParams), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        for (FrameData::DescSets& descSets : frame.mDescriptors)
        {
            writer.updateSet(device, descSets.mCloudDescSet);
        }
        writer.clear();

        std::vector<VkDescriptorImageInfo> tex3dInfos(
            TEXTURE_3D_COUNT, VkDescriptorImageInfo{.sampler = mTextureBank->getSampler(),
                                  .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        tex3dInfos[0].imageView = mMainNoiseTexture.mImageView;
        tex3dInfos[1].imageView = mDetailNoiseTexture.mImageView;
        std::vector<VkDescriptorImageInfo> tex2dInfos(
            TEXTURE_2D_COUNT, VkDescriptorImageInfo{.sampler = mTextureBank->getSampler(),
                                  .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});
        tex2dInfos[0].imageView = mBlueNoiseTexture.mImageView;
        tex2dInfos[1].imageView = mWeatherNoiseTexture.mImageView;

        writer.writeImages(0, tex3dInfos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImages(1, tex2dInfos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(2, frame.mDepthTexture->mImageView, mTextureBank->getSampler(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(3, frame.mCloudTexture2.mImageView, mTextureBank->getSampler(), VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(4, frame.mCloudTexture1.mImageView, mTextureBank->getSampler(), VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); //  can the sampler be nullptr when storage image?
        writer.writeImage(5, frame.mFogShadowTexture.mImageView, mTextureBank->getSampler(), VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, frame.mDescriptors[0].mTextureDescSet);
        writer.clear();

        //  switch the cloud render target and last frame cloud tex
        writer.writeImages(0, tex3dInfos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImages(1, tex2dInfos, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(2, frame.mDepthTexture->mImageView, mTextureBank->getSampler(),
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(3, frame.mCloudTexture1.mImageView, mTextureBank->getSampler(), VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        writer.writeImage(4, frame.mCloudTexture2.mImageView, mTextureBank->getSampler(), VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); //  can the sampler be nullptr when storage image?
        writer.writeImage(5, frame.mFogShadowTexture.mImageView, mTextureBank->getSampler(), VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, frame.mDescriptors[1].mTextureDescSet);
    }

    return BUNNY_HAPPY;
}

BunnyResult SkyPass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding descBinding{
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

    VkDevice device = mVulkanResources->getDevice();

    DescriptorLayoutBuilder builder;
    //  light data
    builder.addBinding(descBinding);
    //  cloud data
    descBinding.binding = 1;
    builder.addBinding(descBinding);
    //  render data
    descBinding.binding = 2;
    builder.addBinding(descBinding);
    mCloudDescSetLayout = builder.build(device);

    builder.clear();
    //  3d textures
    descBinding.binding = 0;
    descBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descBinding.descriptorCount = TEXTURE_3D_COUNT; //  main noise, detail noise
    builder.addBinding(descBinding);
    //  2d textures
    descBinding.binding = 1;
    descBinding.descriptorCount = TEXTURE_2D_COUNT; // blue noise, weather map
    builder.addBinding(descBinding);
    //  scene depth image
    descBinding.binding = 2;
    descBinding.descriptorCount = 1;
    builder.addBinding(descBinding);
    //  last frame cloud texture
    descBinding.binding = 3;
    builder.addBinding(descBinding);
    //  output cloud texture
    descBinding.binding = 4;
    descBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    builder.addBinding(descBinding);
    //  output fog shadow texture
    descBinding.binding = 5;
    builder.addBinding(descBinding);
    mTextureDescSetLayout = builder.build(device);

    mDeletionStack.AddFunction([this, device]() {
        vkDestroyDescriptorSetLayout(device, mCloudDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mTextureDescSetLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

const AllocatedImage& SkyPass::FrameData::getCloudTexture() const
{
    return mCurrentFrameSeqId == 0 ? mCloudTexture1 : mCloudTexture2;
}

} // namespace Bunny::Render
