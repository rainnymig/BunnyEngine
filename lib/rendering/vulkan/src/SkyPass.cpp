#include "SkyPass.h"

#include "Error.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "TextureBank.h"

namespace Bunny::Render
{
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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 2, &frame.mCloudDescSet, 0, nullptr);

    constexpr static uint32_t computeSizeX = 64;
    constexpr static uint32_t computeSizeY = 32;
    VkExtent3D renderImageExtent = frame.mCloudRenderTargetTexture.mExtent;
    vkCmdDispatch(cmd, renderImageExtent.width / computeSizeX, renderImageExtent.height / computeSizeY, 1);
}

void SkyPass::updateFrameData()
{
    //  recreate descriptor sets for binding new data

    //  transition image layouts
    //  swap this and prev cloud image
    //  prev rendered cloud image transition to read
    //  cloud render target of this frame transition to write
    //  fog shadow image to write

    //  get the depth image

    //  descriptor set writes
}

BunnyResult SkyPass::initPipeline()
{
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
    mDescriptorAllocator.init(device, 2, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mCloudDescSetLayout, mTextureDescSetLayout};
    for (FrameData& frame : mFrameData)
    {
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mCloudDescSet, 2);
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
    for (FrameData& frame : mFrameData)
    {
        frame.mCloudRenderTargetTexture =
            mVulkanResources->createImage(renderImageExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mFogShadowTexture = mVulkanResources->createImage(renderImageExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mLastCloudTexture = mVulkanResources->createImage(renderImageExtent, VK_FORMAT_R32G32B32A32_SFLOAT,
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mDepthTexture = &mRenderer->getDepthImage();
    }

    mDeletionStack.AddFunction([this]() {
        for (FrameData& frame : mFrameData)
        {
            mVulkanResources->destroyImage(frame.mCloudRenderTargetTexture);
            mVulkanResources->destroyImage(frame.mFogShadowTexture);
            mVulkanResources->destroyImage(frame.mLastCloudTexture);
            frame.mDepthTexture = nullptr;
        }
    });

    //  load noise and other textures to texture bank
    IdType texId;
    //  main cloud noise
    if (BUNNY_SUCCESS(mTextureBank->addTexture3d("main_cloud_noise.png", VK_FORMAT_R8G8B8A8_UNORM,
            MAIN_CLOUD_NOISE_RESOLUTION, MAIN_CLOUD_NOISE_RESOLUTION, MAIN_CLOUD_NOISE_RESOLUTION, texId)))
    {
        mTextureBank->getTexture3d(texId, mMainNoiseTexture);
    }
    //  detail cloud noise
    if (BUNNY_SUCCESS(mTextureBank->addTexture3d("detail_cloud_noise.png", VK_FORMAT_R8G8B8A8_UNORM,
            DETAIL_CLOUD_NOISE_RESOLUTION, DETAIL_CLOUD_NOISE_RESOLUTION, DETAIL_CLOUD_NOISE_RESOLUTION, texId)))
    {
        mTextureBank->getTexture3d(texId, mDetailNoiseTexture);
    }
    //  weather
    if (BUNNY_SUCCESS(mTextureBank->addTexture("weather.png", VK_FORMAT_R8G8B8A8_UNORM, texId)))
    {
        mTextureBank->getTexture(texId, mWeatherNoiseTexture);
    }
    //  blue noise
    if (BUNNY_SUCCESS(mTextureBank->addTexture("blue_noise.png", VK_FORMAT_R8G8B8A8_UNORM, texId)))
    {
        mTextureBank->getTexture(texId, mBlueNoiseTexture);
    }

    //  init and create cloud and render parameter buffers
    //  init data...
    //  create buffers
    mVulkanResources->createBufferWithData(&mCloudData, sizeof(CloudData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO, mCloudDataBuffer);
    mVulkanResources->createBufferWithData(&mCloudRenderParams, sizeof(CloudRenderParams),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO, mCloudRenderParamsBuffer);

    mDeletionStack.AddFunction([this]() {
        mVulkanResources->destroyBuffer(mCloudDataBuffer);
        mVulkanResources->destroyBuffer(mCloudRenderParamsBuffer);
    });

    //  use the created resources to update the frame descriptor sets

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
    descBinding.descriptorCount = TEXTURE_2D_COUNT; // blue noise, weather map, depth image, previous cloud texture
    builder.addBinding(descBinding);
    //  output cloud texture
    descBinding.binding = 2;
    descBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descBinding.descriptorCount = 1;
    builder.addBinding(descBinding);
    //  output fog shadow texture
    descBinding.binding = 3;
    builder.addBinding(descBinding);
    mTextureDescSetLayout = builder.build(device);

    mDeletionStack.AddFunction([this, device]() {
        vkDestroyDescriptorSetLayout(device, mCloudDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mTextureDescSetLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
