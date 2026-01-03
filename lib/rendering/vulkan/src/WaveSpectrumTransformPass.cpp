#include "WaveSpectrumTransformPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Error.h"
#include "ErrorCheck.h"

namespace Bunny::Render
{
Render::WaveSpectrumTransformPass::WaveSpectrumTransformPass(
    const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer, std::string_view shaderPath)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mShaderPath(shaderPath)
{
}

void Render::WaveSpectrumTransformPass::draw() const
{
    //  vertical ifft

    //  horizontal ifft
}

void Render::WaveSpectrumTransformPass::updateWaveTime(float time)
{
    mFFTParams.mTime = time;
}

void Render::WaveSpectrumTransformPass::updateFFTSize(uint32_t size)
{
    mFFTParams.mN = size;
}

void Render::WaveSpectrumTransformPass::updateSpectrumImage(const AllocatedImage* spectrumImage)
{
    for (FrameData& frame : mFrameData)
    {
        frame.mSpectrumImage = spectrumImage;
    }
}

BunnyResult Render::WaveSpectrumTransformPass::initPipeline()
{
    std::vector<VkDescriptorSetLayout> descLayouts{mImageDescLayout};
    std::vector<VkPushConstantRange> pushConsts;
    VkPushConstantRange pushConst = pushConsts.emplace_back();
    pushConst.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConst.offset = 0;
    pushConst.size = sizeof(FFTParams);
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildComputePipeline(mShaderPath, &descLayouts, &pushConsts))
    return BUNNY_HAPPY;
}

BunnyResult Render::WaveSpectrumTransformPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 2},
    };
    mDescriptorAllocator.init(device, 2, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mImageDescLayout};
    for (FrameData& frame : mFrameData)
    {
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mImageDescSet, 1);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult Render::WaveSpectrumTransformPass::initDataAndResources()
{
    //  create image for fft result

    //  allocate descriptor sets and link image resources to them

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
