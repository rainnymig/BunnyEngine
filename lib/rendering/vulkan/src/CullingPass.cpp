#include "CullingPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Shader.h"
#include "ErrorCheck.h"
#include "Error.h"
#include "ComputePipelineBuilder.h"
#include "Camera.h"
#include "Helper.h"

namespace Bunny::Render
{
CullingPass::CullingPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const MeshBank<NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer),
      mMeshBank(meshBank)
{
}

BunnyResult CullingPass::initializePass()
{
    initDescriptorSets();
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initPipeline())
    createBuffers();

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

    mDrawCommandBuffer = nullptr;
    mVulkanResources->destroyBuffer(mCullingDataBuffer);
}

void CullingPass::createBuffers()
{
    mCullingDataBuffer = mVulkanResources->createBuffer(sizeof(ViewFrustum), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);
}

void CullingPass::linkDrawData(const AllocatedBuffer& drawCommandBuffer, size_t bufferSize)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, drawCommandBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (VkDescriptorSet set : mDrawCommandDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
    mDrawCommandBuffer = &drawCommandBuffer;
}

void CullingPass::linkMeshData(const AllocatedBuffer& meshDataBuffer, size_t bufferSize)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, meshDataBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (VkDescriptorSet set : mMeshDataDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void CullingPass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, objectBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (VkDescriptorSet set : mObjectDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void CullingPass::setObjectCount(uint32_t objectCount)
{
    mObjectCount = objectCount;
}

void CullingPass::updateCullingData(const Camera& camera)
{
    ViewFrustum viewFrustum;
    camera.getViewFrustum(viewFrustum);

    void* data = mCullingDataBuffer.mAllocationInfo.pMappedData;
    memcpy(data, &viewFrustum, sizeof(ViewFrustum));
}

void CullingPass::dispatch()
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();

    //  bind descriptors

    //  dispatch culling compute shader
    constexpr static uint32_t computeSizeX = 256;
    //  +1 to make sure enough threads are dispatched
    vkCmdDispatch(cmd, mObjectCount / computeSizeX + 1, 1, 1);
    //  Todo: maybe research later submitting this to compute queue?

    //  set up barrier to make sure the buffers containing the culling result is ready to use
    VkBufferMemoryBarrier barrier = makeBufferMemoryBarrier(
        mDrawCommandBuffer->mBuffer, mVulkanResources->getGraphicQueue().mQueueFamilyIndex.value());
    barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

    //  Todo: here we directly start the barrier, maybe we should save it somewhere and start it later
    //  in case we want to have multiple culling passes and wait all at the end
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr,
        1, &barrier, 0, nullptr);
}

CullingPass::~CullingPass()
{
    cleanup();
}

void CullingPass::initDescriptorSets()
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
}

BunnyResult CullingPass::initPipeline()
{
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

} // namespace Bunny::Render
