#include "WaveSpectrumTransformPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Error.h"
#include "ErrorCheck.h"

namespace Bunny::Render
{
Render::WaveSpectrumTransformPass::WaveSpectrumTransformPass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, std::string_view spectrumShaderPath, std::string_view fftShaderPath)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mTimedSpectrumShaderPath(spectrumShaderPath),
      mFftShaderPath(fftShaderPath)
{
}

void Render::WaveSpectrumTransformPass::draw() const
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    //  add time to the wave spectrum
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mSpectrumPipeline);
    constexpr static uint32_t spectrumComputeSizeX = 256;
    constexpr static uint32_t spectrumComputeSizeY = 1;
    {
        VkImageMemoryBarrier spectrumImageBarrier =
            makeImageMemoryBarrier(frame.mSpectrumImage->mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier timedSpectrumImageBarrier = makeImageMemoryBarrier(frame.mTimedSpectrumImage.mImage,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier barriers[]{spectrumImageBarrier, timedSpectrumImageBarrier};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);
    }

    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mSpectrumPipelineLayout, 0, 1, &frame.mTimedSpectrumDescSet, 0, nullptr);
    vkCmdPushConstants(cmd, mSpectrumPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, &mTimedSpectrumParams);
    vkCmdDispatch(
        cmd, mTimedSpectrumParams.mN / spectrumComputeSizeX, mTimedSpectrumParams.mN / spectrumComputeSizeY, 1);

    //  fft
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

    constexpr static uint32_t fftComputeSizeX = 256;
    constexpr static uint32_t fftComputeSizeY = 1;

    //  vertical ifft
    {
        VkImageMemoryBarrier timedSpectrumImageBarrier = makeImageMemoryBarrier(frame.mTimedSpectrumImage.mImage,
            VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier intermediateImageBarrier = makeImageMemoryBarrier(frame.mIntermediateFftImage.mImage,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier barriers[]{timedSpectrumImageBarrier, intermediateImageBarrier};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);
    }
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1, &frame.mFirstFftImageDescSet, 0, nullptr);
    mFFTParams.mDirection = 1;
    vkCmdPushConstants(cmd, mPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, &mFFTParams);
    vkCmdDispatch(cmd, mFFTParams.mN / fftComputeSizeX, mFFTParams.mN / fftComputeSizeY, 1);

    //  horizontal ifft
    //  wait for the vertical pass to finish writing to the intermediate image
    {
        VkImageMemoryBarrier intermediateImageBarrier =
            makeImageMemoryBarrier(frame.mIntermediateFftImage.mImage, VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier heightImageBarrier =
            makeImageMemoryBarrier(frame.mWaveHeightImage.mImage, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier barriers[]{intermediateImageBarrier, heightImageBarrier};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);
    }
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1, &frame.mSecondFftImageDescSet, 0, nullptr);
    mFFTParams.mDirection = 0;
    vkCmdPushConstants(cmd, mPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 1, &mFFTParams);
    vkCmdDispatch(cmd, mFFTParams.mN / fftComputeSizeX, mFFTParams.mN / fftComputeSizeY, 1);
}

void Render::WaveSpectrumTransformPass::updateWaveTime(float time)
{
    mTimedSpectrumParams.mTime = time;
    mFFTParams.mTime = time;
}

void Render::WaveSpectrumTransformPass::updateWidth(uint32_t size, float width)
{
    mTimedSpectrumParams.mN = size;
    mTimedSpectrumParams.mWidth = width;
    mFFTParams.mN = size;
}

void Render::WaveSpectrumTransformPass::updateSpectrumImage(const AllocatedImage* spectrumImage)
{
    VkDevice device = mVulkanResources->getDevice();
    DescriptorWriter writer;

    for (FrameData& frame : mFrameData)
    {
        frame.mSpectrumImage = spectrumImage;

        //  link the images of the frame to the descriptor sets
        writer.clear();
        writer.writeImage(0, frame.mSpectrumImage->mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); //  or VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL?
        writer.writeImage(1, frame.mTimedSpectrumImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, frame.mTimedSpectrumDescSet);

        writer.clear();
        writer.writeImage(0, frame.mTimedSpectrumImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); //  or VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL?
        writer.writeImage(1, frame.mIntermediateFftImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, frame.mFirstFftImageDescSet);

        writer.clear();
        writer.writeImage(0, frame.mIntermediateFftImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE); //  or VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL?
        writer.writeImage(
            1, frame.mWaveHeightImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, frame.mFirstFftImageDescSet);
    }
}

BunnyResult Render::WaveSpectrumTransformPass::initPipeline()
{
    std::vector<VkDescriptorSetLayout> descLayouts{mImageDescLayout};
    std::vector<VkPushConstantRange> pushConsts;
    VkPushConstantRange pushConst = pushConsts.emplace_back();

    //  fft pipeline
    pushConst.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConst.offset = 0;
    pushConst.size = sizeof(FFTParams);
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(
        buildComputePipeline(mFftShaderPath, &descLayouts, &pushConsts, &mPipelineLayout, &mPipeline))

    //  timed spectrum pipeline
    pushConst.size = sizeof(TimedSpectrumParams);
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildComputePipeline(
        mTimedSpectrumShaderPath, &descLayouts, &pushConsts, &mSpectrumPipelineLayout, &mSpectrumPipeline))

    return BUNNY_HAPPY;
}

BunnyResult Render::WaveSpectrumTransformPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .mRatio = 2},
    };
    mDescriptorAllocator.init(device, 6, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mImageDescLayout};
    for (FrameData& frame : mFrameData)
    {
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mTimedSpectrumDescSet, 1);
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mFirstFftImageDescSet, 1);
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mSecondFftImageDescSet, 1);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult Render::WaveSpectrumTransformPass::initDataAndResources()
{
    //  create image for fft result
    for (FrameData& frame : mFrameData)
    {
        frame.mTimedSpectrumImage = mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1},
            VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mIntermediateFftImage = mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1},
            VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mWaveHeightImage = mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1},
            VK_FORMAT_R32G32_SFLOAT, VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    mDeletionStack.AddFunction([this]() {
        for (FrameData& frame : mFrameData)
        {
            mVulkanResources->destroyImage(frame.mTimedSpectrumImage);
            mVulkanResources->destroyImage(frame.mIntermediateFftImage);
            mVulkanResources->destroyImage(frame.mWaveHeightImage);
        }
    });

    //  the images will be linked to the descriptor set later after the spectrum image is set

    return BUNNY_HAPPY;
}

BunnyResult Render::WaveSpectrumTransformPass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding descBinding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

    DescriptorLayoutBuilder builder;
    builder.addBinding(descBinding);
    descBinding.binding = 1;
    builder.addBinding(descBinding);
    mImageDescLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction(
        [this]() { vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mImageDescLayout, nullptr); });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
