#include "CullingPass.h"

#include "VulkanRenderResources.h"
#include "Shader.h"
#include "ErrorCheck.h"
#include "ComputePipelineBuilder.h"

namespace Bunny::Render
{
CullingPass::CullingPass(const VulkanRenderResources* vulkanResources) : mVulkanResources(vulkanResources)
{
}

BunnyResult CullingPass::initializePass()
{
    //  build descriptor set layouts
    DescriptorLayoutBuilder layoutBuilder;
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        layoutBuilder.addBinding(uniformBufferLayout);
    }
    mUniformBufferLayout = layoutBuilder.build(mVulkanResources->getDevice());

    layoutBuilder.clear();
    {
        VkDescriptorSetLayoutBinding storageBufferLayout{};
        storageBufferLayout.binding = 0;
        storageBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storageBufferLayout.descriptorCount = 1;
        storageBufferLayout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        storageBufferLayout.pImmutableSamplers = nullptr;
        layoutBuilder.addBinding(storageBufferLayout);
    }
    mStorageBufferLayout = layoutBuilder.build(mVulkanResources->getDevice());

    //  set up descriptor allocator
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .mRatio = 6},
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 2}
    };
    mDescriptorAllocator.init(mVulkanResources->getDevice(), 2, poolSizes);

    //  allocated descriptor sets
    for (size_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mUniformBufferLayout, &mCullDataDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mStorageBufferLayout, &mObjectDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mStorageBufferLayout, &mDrawCommandDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mStorageBufferLayout, &mMeshDataDescSets[idx], 1, nullptr);
    }

    //  load shader
    Shader computeShader(mCullingShaderPath, mVulkanResources->getDevice());

    //  build pipeline layout
    VkDescriptorSetLayout layouts[] = {
        mUniformBufferLayout, mStorageBufferLayout, mStorageBufferLayout, mStorageBufferLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 4;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VK_CHECK_OR_RETURN_BUNNY_SAD(
        vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout))

    //  build pipeline
    ComputePipelineBuilder pipelineBuilder;
    pipelineBuilder.setShader(computeShader.getShaderModule());
    pipelineBuilder.setPipelineLayout(mPipelineLayout);
    mPipeline = pipelineBuilder.build(mVulkanResources->getDevice());

    return BUNNY_HAPPY;
}

void CullingPass::cleanup()
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

    if (mStorageBufferLayout != nullptr)
    {
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mStorageBufferLayout, nullptr);
        mStorageBufferLayout = nullptr;
    }

    if (mUniformBufferLayout != nullptr)
    {
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mUniformBufferLayout, nullptr);
        mUniformBufferLayout = nullptr;
    }
}

void CullingPass::linkCullData(const AllocatedBuffer& cullBuffer)
{
}

void CullingPass::linkDrawData(const AllocatedBuffer& drawCommandBuffer, size_t bufferSize)
{
}

void CullingPass::linkMeshData(const AllocatedBuffer& meshDataBuffer, size_t bufferSize)
{
}

void CullingPass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
}

void CullingPass::dispatch()
{
}

CullingPass::~CullingPass()
{
    cleanup();
}

} // namespace Bunny::Render
