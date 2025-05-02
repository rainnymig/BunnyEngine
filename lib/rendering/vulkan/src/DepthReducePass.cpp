#include "DepthReducePass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Helper.h"
#include "Error.h"
#include "Shader.h"
#include "ComputePipelineBuilder.h"
#include "BunnyResult.h"

#include <glm/vec2.hpp>

namespace Bunny::Render
{

DepthReducePass::DepthReducePass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
}

void DepthReducePass::initializePass()
{
    createDepthHierarchy();
    createDepthReduceSampler();
    initDescriptorSets();
    initPipeline();
}

void DepthReducePass::cleanup()
{
    if (mPipeline != nullptr)
    {
        vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr);
        mPipeline = nullptr;
    }

    if (mPipelineLayout != nullptr)
    {
        vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr);
        mPipelineLayout = nullptr;
    }

    mDescriptorAllocator.destroyPools(mVulkanResources->getDevice());
    vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mDepthDescLayout, nullptr);

    vkDestroySampler(mVulkanResources->getDevice(), mDepthReduceSampler, nullptr);

    for (int idx = 0; idx < mDepthImageViewMips.size(); idx++)
    {
        if (mDepthImageViewMips[idx] != nullptr)
        {
            vkDestroyImageView(mVulkanResources->getDevice(), mDepthImageViewMips[idx], nullptr);
            mDepthImageViewMips[idx] = nullptr;
        }
    }

    mVulkanResources->destroyImage(mdepthHierarchyImage);
}

void DepthReducePass::dispatch()
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();

    const AllocatedImage& depthImage = mRenderer->getDepthImage();

    //  wait for the previous pass to finish writing to the depth image
    VkImageMemoryBarrier depthWriteFinishBarrier =
        makeImageMemoryBarrier(depthImage.mImage, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &depthWriteFinishBarrier);

    //  bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

    for (int32_t idx = 0; idx < mDepthHierarchyLevels; idx++)
    {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1,
            &mDepthDescSets[mRenderer->getCurrentFrameIdx()][idx], 0, nullptr);

        uint32_t levelWidth = mDepthWidth >> idx;
        uint32_t levelHeight = mDepthHeight >> idx;
        if (levelHeight < 1)
        {
            levelHeight = 1;
        }
        if (levelWidth < 1)
        {
            levelWidth = 1;
        }

        glm::vec2 levelSize{levelWidth, levelHeight};

        vkCmdPushConstants(cmd, mPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(glm::vec2), &levelSize);
        vkCmdDispatch(cmd, getGroupCount(levelWidth, 32), getGroupCount(levelHeight, 32), 1);

        //  wait for compute shader to finish one round of reduce before proceeding to the next
        VkImageMemoryBarrier reduceBarrier =
            makeImageMemoryBarrier(mdepthHierarchyImage.mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
                VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &reduceBarrier);
    }

    //  transition the depth image back to the optimal format as depth attachment
    VkImageMemoryBarrier depthReadFinishBarrier = makeImageMemoryBarrier(depthImage.mImage, VK_ACCESS_SHADER_READ_BIT,
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        VK_IMAGE_ASPECT_DEPTH_BIT);

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &depthReadFinishBarrier);
}

void DepthReducePass::createDepthHierarchy()
{
    //  build hierarchical z image from the depth image
    const AllocatedImage& depthImage = mRenderer->getDepthImage();

    mDepthWidth = findPreviousPow2(depthImage.mExtent.width);
    mDepthHeight = findPreviousPow2(depthImage.mExtent.height);
    mDepthHierarchyLevels = findHierarchyLevels(mDepthWidth, mDepthHeight);

    if (mDepthHierarchyLevels > MAX_DEPTH_HIERARCHY_LEVELS)
    {
        PRINT_AND_ABORT("Depth image resolution too high.")
    }

    VkExtent3D hierarchyImageExtent = {static_cast<uint32_t>(mDepthWidth), static_cast<uint32_t>(mDepthHeight), 1};

    //  build the depth hierarchy image
    mdepthHierarchyImage = mVulkanResources->createImage(hierarchyImageExtent, VK_FORMAT_R32_SFLOAT,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, mDepthHierarchyLevels);

    BunnyResult result = mVulkanResources->immediateTransitionImageLayout(mdepthHierarchyImage.mImage,
        mdepthHierarchyImage.mFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, mDepthHierarchyLevels);

    //  build the image view for each mip level
    for (int32_t idx = 0; idx < mDepthHierarchyLevels; idx++)
    {
        VkImageViewCreateInfo viewCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
        viewCreateInfo.pNext = nullptr;
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.image = mdepthHierarchyImage.mImage;
        viewCreateInfo.format = mdepthHierarchyImage.mFormat;
        viewCreateInfo.subresourceRange.baseMipLevel = idx;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        vkCreateImageView(mVulkanResources->getDevice(), &viewCreateInfo, nullptr, &mDepthImageViewMips[idx]);
    }
}

void DepthReducePass::createDepthReduceSampler()
{
    VkSamplerCreateInfo createInfo = {};

    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    createInfo.minLod = 0;
    createInfo.maxLod = MAX_DEPTH_HIERARCHY_LEVELS;

    VkSamplerReductionModeCreateInfoEXT createInfoReduction = {
        VK_STRUCTURE_TYPE_SAMPLER_REDUCTION_MODE_CREATE_INFO_EXT};

    VkSamplerReductionMode reductionMode = VK_SAMPLER_REDUCTION_MODE_MAX;
    if (reductionMode != VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE_EXT)
    {
        createInfoReduction.reductionMode = reductionMode;
        createInfo.pNext = &createInfoReduction;
    }

    vkCreateSampler(mVulkanResources->getDevice(), &createInfo, 0, &mDepthReduceSampler);
}

void DepthReducePass::initDescriptorSets()
{
    //  build descriptor set layouts
    DescriptorLayoutBuilder builder;
    VkDescriptorSetLayoutBinding outImageBinding{};
    outImageBinding.binding = 0;
    outImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    outImageBinding.descriptorCount = 1;
    outImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    outImageBinding.pImmutableSamplers = nullptr;
    VkDescriptorSetLayoutBinding inImageBinding{};
    inImageBinding.binding = 1;
    inImageBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    inImageBinding.descriptorCount = 1;
    inImageBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    inImageBinding.pImmutableSamplers = nullptr;
    builder.addBinding(outImageBinding);
    builder.addBinding(inImageBinding);
    mDepthDescLayout = builder.build(mVulkanResources->getDevice());

    //  setup descriptor allocator
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          .mRatio = 2},
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 2}
    };
    mDescriptorAllocator.init(mVulkanResources->getDevice(), MAX_DEPTH_HIERARCHY_LEVELS, poolSizes);

    const AllocatedImage& depthImage = mRenderer->getDepthImage();

    //  allocate and link descriptor sets
    for (auto& frameDepthDescSets : mDepthDescSets)
    {
        for (int32_t idx = 0; idx < mDepthHierarchyLevels; idx++)
        {
            mDescriptorAllocator.allocate(
                mVulkanResources->getDevice(), &mDepthDescLayout, &frameDepthDescSets[idx], 1, nullptr);

            //  link depth images
            DescriptorWriter writer;
            writer.writeImage(0, mDepthImageViewMips[idx], mDepthReduceSampler, VK_IMAGE_LAYOUT_GENERAL,
                VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            if (idx == 0)
            {
                writer.writeImage(1, depthImage.mImageView, mDepthReduceSampler,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            }
            else
            {
                writer.writeImage(1, mDepthImageViewMips[idx - 1], mDepthReduceSampler, VK_IMAGE_LAYOUT_GENERAL,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            }
            writer.updateSet(mVulkanResources->getDevice(), frameDepthDescSets[idx]);
        }
    }
}

void DepthReducePass::initPipeline()
{
    Shader computeShader(mShaderPath, mVulkanResources->getDevice());

    VkPushConstantRange pushConstRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof(glm::vec2)};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &mDepthDescLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;

    vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout);

    ComputePipelineBuilder pipelineBuilder;
    pipelineBuilder.setShader(computeShader.getShaderModule());
    pipelineBuilder.setPipelineLayout(mPipelineLayout);
    mPipeline = pipelineBuilder.build(mVulkanResources->getDevice());
}

uint32_t DepthReducePass::findHierarchyLevels(uint32_t width, uint32_t height) const
{
    uint32_t result = 1;

    while (width > 1 || height > 1)
    {
        result++;
        width /= 2;
        height /= 2;
    }

    return result;
}

uint32_t Render::DepthReducePass::getGroupCount(uint32_t threadCount, uint32_t localSize) const
{
    return (threadCount + localSize - 1) / localSize;
}

} // namespace Bunny::Render
