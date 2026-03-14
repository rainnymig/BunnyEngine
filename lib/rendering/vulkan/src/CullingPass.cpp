#include "CullingPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Shader.h"
#include "ErrorCheck.h"
#include "Error.h"
#include "ComputePipelineBuilder.h"
#include "Camera.h"
#include "Helper.h"

#include <array>

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

    if (mCullDataLayout != nullptr)
    {
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mCullDataLayout, nullptr);
        mCullDataLayout = nullptr;
    }

    if (mMeshDataLayout != nullptr)
    {
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mMeshDataLayout, nullptr);
        mMeshDataLayout = nullptr;
    }

    if (mDrawDataLayout != nullptr)
    {
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mDrawDataLayout, nullptr);
        mDrawDataLayout = nullptr;
    }

    mDrawCommandBuffer = nullptr;
    mInstanceObjectBuffer = nullptr;
    mVulkanResources->destroyBuffer(mCullingDataBuffer);
    mVulkanResources->destroyBuffer(mDebugDataBuffer);
}

void CullingPass::createBuffers()
{
    VkDeviceSize bufferSize = sizeof(ViewFrustum);
    mCullingDataBuffer = mVulkanResources->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);
}

void CullingPass::linkDrawData(const AllocatedBuffer& drawCommandBuffer, size_t drawbufferSize,
    const AllocatedBuffer& instObjectBuffer, size_t instBufferSize)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, drawCommandBuffer.mBuffer, drawbufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writer.writeBuffer(1, instObjectBuffer.mBuffer, instBufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (VkDescriptorSet set : mDrawCommandDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
    mDrawCommandBuffer = &drawCommandBuffer;
    mInstanceObjectBuffer = &instObjectBuffer;
}

void CullingPass::linkMeshData()
{
    DescriptorWriter writer;
    writer.writeBuffer(0, mMeshBank->getMeshDataBuffer().mBuffer, mMeshBank->getMeshDataBufferSize(), 0,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    writer.writeBuffer(1, mMeshBank->getSurfaceDataBuffer().mBuffer, mMeshBank->getSurfaceDataBufferSize(), 0,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
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

void CullingPass::linkCullingData(
    const std::array<const AllocatedImage*, MAX_FRAMES_IN_FLIGHT>& depthHierarchyImages, VkSampler sampler)
{
    mDepthHierarchyImages = depthHierarchyImages;

    //  link culling data buffer
    DescriptorWriter writer;
    VkDeviceSize bufferSize = sizeof(ViewFrustum);

    for (uint32_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        //  note that the depth hierarchy image of the same frame idx is used for each frame
        //  this means that the depth hierarchy used in the culling is not of the previous frame
        //  but of MAX_FRAMES_IN_FLIGHT frames earlier
        //  this should not be a big issue
        //  if want to use the previous 1 frame we should use (idx-1)%MAX_FRAMES_IN_FLIGHT

        writer.clear();
        writer.writeBuffer(0, mCullingDataBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.writeImage(1, depthHierarchyImages[idx]->mImageView, sampler, VK_IMAGE_LAYOUT_GENERAL,
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        writer.updateSet(mVulkanResources->getDevice(), mCullDataDescSets[idx]);
    }
}

void CullingPass::setObjectCount(uint32_t objectCount)
{
    mObjectCount = objectCount;

    VkDeviceSize bufferSize = sizeof(glm::vec4) * objectCount;
    mDebugDataBuffer = mVulkanResources->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_MEMORY_USAGE_AUTO);
    DescriptorWriter writer;
    writer.writeBuffer(0, mDebugDataBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (VkDescriptorSet set : mDebugDataDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void CullingPass::setDepthImageSizes(uint32_t width, uint32_t height, uint32_t levels)
{
    mDepthImageWidth = width;
    mDepthImageHeight = height;
    mDepthMipLevels = levels;
}

void CullingPass::updateCullingData(const Camera& camera)
{
    ViewFrustum viewFrustum;
    camera.getViewFrustum(viewFrustum);
    viewFrustum.mDepthImageWidth = mDepthImageWidth;
    viewFrustum.mDepthImageHeight = mDepthImageHeight;

    void* data = mCullingDataBuffer.mAllocationInfo.pMappedData;
    memcpy(data, &viewFrustum, sizeof(ViewFrustum));
}

void CullingPass::dispatch()
{
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    uint32_t currentFrameIdx = mRenderer->getCurrentFrameIdx();

    //  bind pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipeline);

    //  bind descriptors
    //  cull data
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 0, 1, &mCullDataDescSets[currentFrameIdx], 0, nullptr);

    //  object data
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 1, 1, &mObjectDescSets[currentFrameIdx], 0, nullptr);

    //  mesh data
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 2, 1, &mMeshDataDescSets[currentFrameIdx], 0, nullptr);

    //  indirect draw data
    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 3, 1, &mDrawCommandDescSets[currentFrameIdx], 0, nullptr);

    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_COMPUTE, mPipelineLayout, 4, 1, &mDebugDataDescSets[currentFrameIdx], 0, nullptr);

    vkCmdPushConstants(cmd, mPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(uint32_t), &mObjectCount);

    {
        //  barrier for depth hierarchy image write
        VkImageMemoryBarrier depthHierarchyBarrier = makeImageMemoryBarrier(
            mDepthHierarchyImages[currentFrameIdx]->mImage, VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_ASPECT_COLOR_BIT);
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0,
            nullptr, 0, nullptr, 1, &depthHierarchyBarrier);
    }

    //  dispatch culling compute shader
    constexpr static uint32_t computeSizeX = 256;
    //  +1 to make sure enough threads are dispatched
    vkCmdDispatch(cmd, mObjectCount / computeSizeX + 1, 1, 1);
    //  Todo: maybe research later submitting this to compute queue?

    //  set up barrier to make sure the buffers containing the culling result is ready to use
    std::array<VkBufferMemoryBarrier, 2> barriers;

    barriers[0] = makeBufferMemoryBarrier(
        mDrawCommandBuffer->mBuffer, mVulkanResources->getGraphicQueue().mQueueFamilyIndex.value());
    barriers[0].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barriers[0].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

    barriers[1] = makeBufferMemoryBarrier(
        mInstanceObjectBuffer->mBuffer, mVulkanResources->getGraphicQueue().mQueueFamilyIndex.value());
    barriers[1].srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    barriers[1].dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;

    //  Todo: here we directly start the barrier, maybe we should save it somewhere and start it later
    //  in case we want to have multiple culling passes and wait all at the end
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, 0, 0, nullptr,
        2, barriers.data(), 0, nullptr);
}

CullingPass::~CullingPass()
{
    cleanup();
}

void CullingPass::initDescriptorSets()
{
    //  build descriptor set layouts
    DescriptorLayoutBuilder layoutBuilder;

    VkDescriptorSetLayoutBinding uniformBufferBinding{
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutBinding imageBinding{
        1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    VkDescriptorSetLayoutBinding storageBufferBinding{
        0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

    //  culling data
    layoutBuilder.addBinding(uniformBufferBinding);
    //  depth hierarchy images
    layoutBuilder.addBinding(imageBinding);
    mCullDataLayout = layoutBuilder.build(mVulkanResources->getDevice());

    layoutBuilder.clear();
    //  various storage buffers
    layoutBuilder.addBinding(storageBufferBinding);
    mStorageBufferLayout = layoutBuilder.build(mVulkanResources->getDevice());

    layoutBuilder.clear();
    //  mesh data
    storageBufferBinding.binding = 0;
    layoutBuilder.addBinding(storageBufferBinding);
    //  surface data
    storageBufferBinding.binding = 1;
    layoutBuilder.addBinding(storageBufferBinding);
    mMeshDataLayout = layoutBuilder.build(mVulkanResources->getDevice());

    layoutBuilder.clear();
    //  draw commands
    storageBufferBinding.binding = 0;
    layoutBuilder.addBinding(storageBufferBinding);
    //  instance to obj id
    storageBufferBinding.binding = 1;
    layoutBuilder.addBinding(storageBufferBinding);
    mDrawDataLayout = layoutBuilder.build(mVulkanResources->getDevice());

    //  set up descriptor allocator
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .mRatio = 10},
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 2 },
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 2 }
    };
    mDescriptorAllocator.init(mVulkanResources->getDevice(), 2, poolSizes);

    //  allocated descriptor sets
    for (size_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mCullDataLayout, &mCullDataDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mStorageBufferLayout, &mObjectDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mMeshDataLayout, &mMeshDataDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mDrawDataLayout, &mDrawCommandDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mStorageBufferLayout, &mDebugDataDescSets[idx], 1, nullptr);
    }
}

BunnyResult CullingPass::initPipeline()
{
    //  load shader
    Shader computeShader(mCullingShaderPath, mVulkanResources->getDevice());

    //  build pipeline layout
    VkDescriptorSetLayout layouts[] = {
        mCullDataLayout, mStorageBufferLayout, mMeshDataLayout, mDrawDataLayout, mStorageBufferLayout};

    VkPushConstantRange pushConstRange{
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = sizeof(uint32_t)};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 5;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstRange;

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
