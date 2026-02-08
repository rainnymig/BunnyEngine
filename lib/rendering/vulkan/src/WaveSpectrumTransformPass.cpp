#include "WaveSpectrumTransformPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Error.h"
#include "ErrorCheck.h"

namespace Bunny::Render
{

static constexpr uint32_t DIRECTION_ROW = 0;
static constexpr uint32_t DIRECTION_COLUMN = 1;

Render::WaveSpectrumTransformPass::WaveSpectrumTransformPass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, uint32_t size, float width, std::string_view spectrumShaderPath,
    std::string_view bitReverseShaderPath, std::string_view fftPingPongShaderPath)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mTimedSpectrumShaderPath(spectrumShaderPath),
      mBitReverseShaderPath(bitReverseShaderPath),
      mFftShaderPath(fftPingPongShaderPath)
{
    updateWidth(size, width);
}

void Render::WaveSpectrumTransformPass::draw() const
{
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    VkDevice device = mVulkanResources->getDevice();

    frame.mDescriptorAllocator.clearPools(device);

    //  add time to the wave spectrum
    computeTimedSpectrum();

    //  transform the spectrums to get the actual values
    //  should not matter, but only one is setting the frame.mIsOutputToImagePong
    fastFourierTransform(
        frame.mTimedSpectrumPing, frame.mTimedSpectrumPong, mFFTParams.mN, true, frame.mIsOutputToImagePong);
    {
        bool dummyBool;
        fastFourierTransform(frame.mSlopeSpectrumPing, frame.mSlopeSpectrumPong, mFFTParams.mN, true, dummyBool);
    }
    {
        bool dummyBool;
        fastFourierTransform(frame.mDisplaceSpectrumPing, frame.mDisplaceSpectrumPong, mFFTParams.mN, true, dummyBool);
    }
    {
        bool dummyBool;
        fastFourierTransform(
            frame.mDisplaceSlopeSpectrumPing, frame.mDisplaceSlopeSpectrumPong, mFFTParams.mN, true, dummyBool);
    }

    //  use the inverse FFT result to construct the actual wave vertex displacement and normal
    constructWave();
}

void Render::WaveSpectrumTransformPass::updateWaveTime(float time)
{
    mTimedSpectrumParams.mTime = time;
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
    }
}

void WaveSpectrumTransformPass::prepareCurrentFrameImagesForView()
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    VkImageMemoryBarrier heightImageBarrier = makeImageMemoryBarrier(
        frame.mIsOutputToImagePong ? frame.mTimedSpectrumPong.mImage : frame.mTimedSpectrumPing.mImage,
        VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    VkImageMemoryBarrier barriers[]{heightImageBarrier};
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, barriers);
}

const AllocatedImage& WaveSpectrumTransformPass::getHeightImage() const
{
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    return frame.mIsOutputToImagePong ? frame.mTimedSpectrumPong : frame.mTimedSpectrumPing;
}

BunnyResult Render::WaveSpectrumTransformPass::initPipeline()
{
    std::vector<VkDescriptorSetLayout> descLayouts{mImageDescLayout};
    std::vector<VkPushConstantRange> pushConsts;
    VkPushConstantRange& pushConst = pushConsts.emplace_back();
    pushConst.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConst.offset = 0;

    //  fft pipelines
    //  bit reverse
    pushConst.size = sizeof(FFTParams);
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildComputePipeline(
        mBitReverseShaderPath, &descLayouts, &pushConsts, &mBitReversePipelineLayout, &mBitReversePipeline))

    //  pingpong
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(
        buildComputePipeline(mFftShaderPath, &descLayouts, &pushConsts, &mPipelineLayout, &mPipeline))

    //  timed spectrum pipeline
    std::vector<VkDescriptorSetLayout> specDescLayouts{mTimedSpectrumDescLayout};
    pushConst.size = sizeof(TimedSpectrumParams);
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildComputePipeline(
        mTimedSpectrumShaderPath, &specDescLayouts, &pushConsts, &mSpectrumPipelineLayout, &mSpectrumPipeline))

    return BUNNY_HAPPY;
}

BunnyResult Render::WaveSpectrumTransformPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .mRatio = 2},
    };

    for (FrameData& frame : mFrameData)
    {
        frame.mDescriptorAllocator.init(device, 6, poolSizes);
    }

    //  all desc sets will be allocated and written to when drawing every frame

    mDeletionStack.AddFunction([this]() {
        for (FrameData& frame : mFrameData)
        {
            frame.mDescriptorAllocator.destroyPools(mVulkanResources->getDevice());
        }
    });

    return BUNNY_HAPPY;
}

BunnyResult Render::WaveSpectrumTransformPass::initDataAndResources()
{
    //  create image for fft result
    for (FrameData& frame : mFrameData)
    {
        frame.mTimedSpectrumPing =
            mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1}, VK_FORMAT_R32G32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mTimedSpectrumPong =
            mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1}, VK_FORMAT_R32G32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mSlopeSpectrumPing =
            mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1}, VK_FORMAT_R32G32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mSlopeSpectrumPong =
            mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1}, VK_FORMAT_R32G32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mDisplaceSpectrumPing =
            mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1}, VK_FORMAT_R32G32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mDisplaceSpectrumPong =
            mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1}, VK_FORMAT_R32G32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mDisplaceSlopeSpectrumPing =
            mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1}, VK_FORMAT_R32G32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        frame.mDisplaceSlopeSpectrumPong =
            mVulkanResources->createImage(VkExtent3D{mFFTParams.mN, mFFTParams.mN, 1}, VK_FORMAT_R32G32_SFLOAT,
                VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    mDeletionStack.AddFunction([this]() {
        for (FrameData& frame : mFrameData)
        {
            mVulkanResources->destroyImage(frame.mTimedSpectrumPing);
            mVulkanResources->destroyImage(frame.mTimedSpectrumPong);
            mVulkanResources->destroyImage(frame.mSlopeSpectrumPing);
            mVulkanResources->destroyImage(frame.mSlopeSpectrumPong);
            mVulkanResources->destroyImage(frame.mDisplaceSpectrumPing);
            mVulkanResources->destroyImage(frame.mDisplaceSpectrumPong);
            mVulkanResources->destroyImage(frame.mDisplaceSlopeSpectrumPing);
            mVulkanResources->destroyImage(frame.mDisplaceSlopeSpectrumPong);
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
    builder.addBinding(descBinding); //  input spectrum
    descBinding.binding = 1;
    builder.addBinding(descBinding); //  timed spectrum
    descBinding.binding = 2;
    builder.addBinding(descBinding); //  slope spectrum
    descBinding.binding = 3;
    builder.addBinding(descBinding); //  displacement spectrum
    descBinding.binding = 4;
    builder.addBinding(descBinding); //  displacement slope spectrum
    mTimedSpectrumDescLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction(
        [this]() { vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mTimedSpectrumDescLayout, nullptr); });

    DescriptorLayoutBuilder builder;
    builder.addBinding(descBinding);
    descBinding.binding = 1;
    builder.addBinding(descBinding);
    mImageDescLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction(
        [this]() { vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mImageDescLayout, nullptr); });

    return BUNNY_HAPPY;
}

void WaveSpectrumTransformPass::computeTimedSpectrum() const
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    VkDevice device = mVulkanResources->getDevice();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mSpectrumPipeline);

    VkDescriptorSet spectrumDescSet;
    frame.mDescriptorAllocator.allocate(device, &mTimedSpectrumDescLayout, &spectrumDescSet, 1);
    DescriptorWriter writer;
    writer.writeImage(
        0, frame.mSpectrumImage->mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.writeImage(
        1, frame.mTimedSpectrumPing.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.writeImage(
        2, frame.mSlopeSpectrumPing.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.writeImage(3, frame.mDisplaceSlopeSpectrumPing.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.writeImage(4, frame.mDisplaceSlopeSpectrumPing.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.updateSet(device, spectrumDescSet);

    constexpr static uint32_t spectrumComputeSizeX = 16;
    constexpr static uint32_t spectrumComputeSizeY = 16;
    {
        VkImageMemoryBarrier spectrumImageBarrier =
            makeImageMemoryBarrier(frame.mSpectrumImage->mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier timedSpectrumImageBarrier = makeImageMemoryBarrier(frame.mTimedSpectrumPing.mImage,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier slopeSpectrumImageBarrier = makeImageMemoryBarrier(frame.mSlopeSpectrumPing.mImage,
            VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier displacementSpectrumImageBarrier = makeImageMemoryBarrier(
            frame.mDisplaceSpectrumPing.mImage, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier displacementSlopeSpectrumImageBarrier = makeImageMemoryBarrier(
            frame.mDisplaceSlopeSpectrumPing.mImage, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier barriers[]{spectrumImageBarrier, timedSpectrumImageBarrier, slopeSpectrumImageBarrier,
            displacementSpectrumImageBarrier, displacementSlopeSpectrumImageBarrier};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 5, barriers);
    }

    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mSpectrumPipelineLayout, 0, 1, &spectrumDescSet, 0, nullptr);
    vkCmdPushConstants(cmd, mSpectrumPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(TimedSpectrumParams),
        &mTimedSpectrumParams);
    vkCmdDispatch(
        cmd, mTimedSpectrumParams.mN / spectrumComputeSizeX, mTimedSpectrumParams.mN / spectrumComputeSizeY, 1);
}

void Render::WaveSpectrumTransformPass::fastFourierTransform(const AllocatedImage& inputImage,
    const AllocatedImage& bufferImage, uint32_t N, bool isInverse, bool& isOutputToBuffer) const
{
    fastFourierTransformOneDir(inputImage, bufferImage, N, DIRECTION_COLUMN, isInverse, isOutputToBuffer);
    if (isOutputToBuffer)
    {
        fastFourierTransformOneDir(bufferImage, inputImage, N, DIRECTION_ROW, isInverse, isOutputToBuffer);

        //  tricky here! because the "buffer" in this call to OneDir is actually the inputImage,
        //  so when passing the value out the bool should be inverted
        isOutputToBuffer = !isOutputToBuffer;
    }
    else
    {
        fastFourierTransformOneDir(inputImage, bufferImage, N, DIRECTION_ROW, isInverse, isOutputToBuffer);
    }
}

void Render::WaveSpectrumTransformPass::fastFourierTransformOneDir(const AllocatedImage& inputImage,
    const AllocatedImage& bufferImage, uint32_t N, uint32_t direction, bool isInverse, bool& isOutputToBuffer) const
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    VkDevice device = mVulkanResources->getDevice();

    FFTParams fftParams = mFFTParams;
    fftParams.mIsInverse = isInverse ? 1 : 0;
    fftParams.mN = N;

    uint32_t maxIteration = std::log2<uint32_t>(N);

    //  bind bit reverse permutation pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mBitReversePipeline);

    constexpr static uint32_t bitReverseSizeX = 16;
    constexpr static uint32_t bitReverseSizeY = 16;

    //  allocate descriptor sets and write images to them
    VkDescriptorSet inputToBufferDescSet;
    VkDescriptorSet bufferToInputDescSet;
    frame.mDescriptorAllocator.allocate(device, &mImageDescLayout, &inputToBufferDescSet, 1);
    frame.mDescriptorAllocator.allocate(device, &mImageDescLayout, &bufferToInputDescSet, 1);
    {
        DescriptorWriter descWriter;
        descWriter.writeImage(
            0, inputImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        descWriter.writeImage(
            1, bufferImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        descWriter.updateSet(device, inputToBufferDescSet);
        descWriter.clear();
        descWriter.writeImage(
            0, bufferImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        descWriter.writeImage(
            1, inputImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        descWriter.updateSet(device, bufferToInputDescSet);
    }

    {
        VkImageMemoryBarrier inputImageBarrier = makeImageMemoryBarrier(inputImage.mImage, VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier outputImageBarrier = makeImageMemoryBarrier(bufferImage.mImage, VK_ACCESS_SHADER_READ_BIT,
            VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        VkImageMemoryBarrier barriers[]{inputImageBarrier, outputImageBarrier};
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);
    }

    //  need to use correct layout!!
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mBitReversePipelineLayout, 0, 1, &inputToBufferDescSet, 0, nullptr);
    fftParams.mDirection = direction;
    vkCmdPushConstants(cmd, mBitReversePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FFTParams), &fftParams);
    vkCmdDispatch(cmd, mFFTParams.mN / bitReverseSizeX, mFFTParams.mN / bitReverseSizeY, 1);

    constexpr static uint32_t fftSizeX = 64;
    constexpr static uint32_t fftSizeY = 1;

    //  bind fft pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

    isOutputToBuffer = true;
    fftParams.iteration = 1;
    while (fftParams.iteration <= maxIteration)
    {
        isOutputToBuffer = !isOutputToBuffer;

        {
            VkImageMemoryBarrier inputImageBarrier = makeImageMemoryBarrier(
                isOutputToBuffer ? inputImage.mImage : bufferImage.mImage, VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
            VkImageMemoryBarrier outputImageBarrier =
                makeImageMemoryBarrier(isOutputToBuffer ? bufferImage.mImage : inputImage.mImage,
                    VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
                    VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
            VkImageMemoryBarrier barriers[]{inputImageBarrier, outputImageBarrier};
            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 2, barriers);
        }
        //  need to use correct pipeline layout!!!
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1,
            isOutputToBuffer ? &inputToBufferDescSet : &bufferToInputDescSet, 0, nullptr);
        vkCmdPushConstants(cmd, mPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(FFTParams), &fftParams);
        vkCmdDispatch(cmd, mFFTParams.mN / fftSizeX, mFFTParams.mN / fftSizeY, 1);

        fftParams.iteration++;
    }
}

void WaveSpectrumTransformPass::constructWave() const
{
}

} // namespace Bunny::Render
