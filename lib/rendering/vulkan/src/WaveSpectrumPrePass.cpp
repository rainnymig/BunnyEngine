#include "WaveSpectrumPrePass.h"

#include "Error.h"
#include "ErrorCheck.h"
#include "Shader.h"
#include "Descriptor.h"
#include "ComputePipelineBuilder.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Helper.h"

#include <glm/vec4.hpp>

#include <array>
#include <vector>
#include <random>

namespace Bunny::Render
{

WaveSpectrumPrePass::WaveSpectrumPrePass(
    const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer, std::string_view shaderPath)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mShaderPath(shaderPath)
{
}

void WaveSpectrumPrePass::draw() const
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 2, &mFrame.mImageDescSet, 0, nullptr);

    constexpr static uint32_t computeSizeX = 16;
    constexpr static uint32_t computeSizeY = 16;

    VkImageMemoryBarrier spectrumImagePreBarrier =
        makeImageMemoryBarrier(mSpectrumImage.mImage, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &spectrumImagePreBarrier);

    vkCmdDispatch(cmd, mWaveSpectrumData.N / computeSizeX, mWaveSpectrumData.N / computeSizeY, 1);

    // VkImageMemoryBarrier spectrumImagePostBarrier =
    //     makeImageMemoryBarrier(mSpectrumImage.mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
    //         VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    // vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    //     VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &spectrumImagePostBarrier);
}

void WaveSpectrumPrePass::prepareSpectrumImageForView()
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();

    VkImageMemoryBarrier spectrumImagePostBarrier =
        makeImageMemoryBarrier(mSpectrumImage.mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &spectrumImagePostBarrier);
}

BunnyResult WaveSpectrumPrePass::initPipeline()
{
    std::vector<VkDescriptorSetLayout> descLayouts{mImageDescLayout, mSpectrumDescLayout};
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(
        buildComputePipeline(mShaderPath, &descLayouts, nullptr, &mPipelineLayout, &mPipeline))
    return BUNNY_HAPPY;
}

BunnyResult WaveSpectrumPrePass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 2},
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,  .mRatio = 2},
    };
    mDescriptorAllocator.init(device, 4, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mImageDescLayout, mSpectrumDescLayout};
    mDescriptorAllocator.allocate(device, descLayouts, &mFrame.mImageDescSet, 2);

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult WaveSpectrumPrePass::initDataAndResources()
{
    //  create spectrum data and buffer
    mWaveSpectrumData.wind = glm::vec2(5.0f, 3.0f);
    mWaveSpectrumData.width = AREA_WIDTH;
    mWaveSpectrumData.N = GRID_N;
    // mWaveSpectrumData.A = 0.81f / (AREA_WIDTH * AREA_WIDTH);
    mWaveSpectrumData.A = 4;

    mVulkanResources->createBufferWithData(&mWaveSpectrumData, sizeof(WaveSpectrumData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO, mWaveSpectrumBuffer);

    //  create spectrum image
    mSpectrumImage = mVulkanResources->createImage({GRID_N, GRID_N, 1}, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT, false,
        VK_IMAGE_LAYOUT_GENERAL);

    mDeletionStack.AddFunction([this]() {
        mVulkanResources->destroyBuffer(mWaveSpectrumBuffer);
        mVulkanResources->destroyImage(mSpectrumImage);
    });

    //  create image containing random values drawn from standard normal distribution
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(createRandomValueImage())

    //  link descriptor sets
    DescriptorWriter writer;
    writer.writeImage(0, mSpectrumImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.writeImage(
        1, mStdNormalDistImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    writer.updateSet(mVulkanResources->getDevice(), mFrame.mImageDescSet);

    writer.clear();
    writer.writeBuffer(0, mWaveSpectrumBuffer.mBuffer, sizeof(WaveSpectrumData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.updateSet(mVulkanResources->getDevice(), mFrame.mSpectrumDescSet);

    return BUNNY_HAPPY;
}

BunnyResult WaveSpectrumPrePass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding descBinding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

    VkDevice device = mVulkanResources->getDevice();

    DescriptorLayoutBuilder builder;
    builder.addBinding(descBinding); //  out spectrum image
    descBinding.binding = 1;
    builder.addBinding(descBinding); //  standard normal distribution random values
    mImageDescLayout = builder.build(device);

    builder.clear();
    descBinding.binding = 0;
    descBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    builder.addBinding(descBinding); //  wave spectrum params
    mSpectrumDescLayout = builder.build(device);

    mDeletionStack.AddFunction([this]() {
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mImageDescLayout, nullptr);
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mSpectrumDescLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

BunnyResult WaveSpectrumPrePass::createRandomValueImage()
{
    //  create random value image data
    size_t size = GRID_N * GRID_N;
    std::vector<glm::vec4> dataVec;
    dataVec.reserve(size);

    std::random_device rd{};
    std::mt19937 gen(rd());
    std::normal_distribution<float> stdNormal(0, 1);

    for (size_t i = 0; i < size; i++)
    {
        dataVec.emplace_back(stdNormal(gen), stdNormal(gen), stdNormal(gen), stdNormal(gen));
    }

    //  create image
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(mVulkanResources->createImageWithData(dataVec.data(),
        getContainerDataSize(dataVec), VkExtent3D{GRID_N, GRID_N, 1}, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL, mStdNormalDistImage))

    //  clean up
    mDeletionStack.AddFunction([this]() { mVulkanResources->destroyImage(mStdNormalDistImage); });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
