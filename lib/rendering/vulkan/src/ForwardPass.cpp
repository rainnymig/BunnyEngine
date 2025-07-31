#include "ForwardPass.h"

#include "Material.h"
#include "MaterialBank.h"
#include "VulkanGraphicsRenderer.h"
#include "VulkanRenderResources.h"
#include "ShaderData.h"
#include "Helper.h"

#include <volk.h>

#include <vector>
#include "DeferredShadingPass.h"

namespace Bunny::Render
{
ForwardPass::ForwardPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const MaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer),
      mMaterialBank(materialBank),
      mMeshBank(meshBank)
{
}

void ForwardPass::initializePass(
    VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout objectLayout, VkDescriptorSetLayout drawLayout)
{
    mSceneDescLayout = sceneLayout;
    mObjectDescLayout = objectLayout;
    mDrawDescLayout = drawLayout;

    //  create descriptor for object buffer and scene buffer
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .mRatio = 4},
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 6}
    };
    mDescriptorAllocator.init(mVulkanResources->getDevice(), 4, poolSizes);

    for (size_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mObjectDescLayout, &mObjectDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mSceneDescLayout, &mSceneDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mDrawDescLayout, &mDrawDataDescSets[idx], 1, nullptr);
    }
}

void ForwardPass::buildDrawCommands()
{
    const std::vector<MeshLite>& meshes = mMeshBank->getMeshes();
    mDrawCommandsData.clear();

    for (const MeshLite& mesh : meshes)
    {
        for (const SurfaceLite& surface : mesh.mSurfaces)
        {
            //  add a draw indirect command for each surface in the mesh
            mDrawCommandsData.emplace_back(surface.mIndexCount, 0, surface.mFirstIndex, mesh.mVertexOffset, 0);
        }
    }

    const VkDeviceSize drawCommandsSize = mDrawCommandsData.size() * sizeof(VkDrawIndexedIndirectCommand);

    mInitialDrawCommandBuffer = mVulkanResources->createBuffer(drawCommandsSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    mDrawCommandsBuffer = mVulkanResources->createBuffer(drawCommandsSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_MEMORY_USAGE_AUTO);
}

void ForwardPass::updateDrawInstanceCounts(std::unordered_map<IdType, size_t> meshInstanceCounts)
{
    const std::vector<MeshLite>& meshes = mMeshBank->getMeshes();

    size_t idx = 0;
    size_t accumulatedInstances = 0;
    for (const MeshLite& mesh : meshes)
    {
        for (const SurfaceLite& surface : mesh.mSurfaces)
        {
            mDrawCommandsData[idx].firstInstance = accumulatedInstances;
            idx++;
        }
        accumulatedInstances += meshInstanceCounts.at(mesh.mId);
    }

    const VkDeviceSize drawCommandsSize = mDrawCommandsData.size() * sizeof(VkDrawIndexedIndirectCommand);
    void* mappedData = mInitialDrawCommandBuffer.mAllocationInfo.pMappedData;
    memcpy(mappedData, mDrawCommandsData.data(), drawCommandsSize);

    //  create instance to object buffer
    //  the number of items in the array is the total number of instances of all meshes
    {
        mVulkanResources->destroyBuffer(mInstanceObjectBuffer);
        const VkDeviceSize bufferSize = sizeof(uint32_t) * accumulatedInstances;
        mInstanceObjectBuffer = mVulkanResources->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_MEMORY_USAGE_AUTO);
        mInstanceObjectBufferSize = bufferSize;

        //  link instance object buffer to descriptor
        DescriptorWriter writer;
        writer.writeBuffer(0, mInstanceObjectBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        for (VkDescriptorSet set : mDrawDataDescSets)
        {
            writer.updateSet(mVulkanResources->getDevice(), set);
        }
    }
}

void ForwardPass::resetDrawCommands()
{
    const VkDeviceSize drawCommandsSize = mDrawCommandsData.size() * sizeof(VkDrawIndexedIndirectCommand);
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();

    VkBufferCopy bufCopy{0};
    bufCopy.dstOffset = 0;
    bufCopy.srcOffset = 0;
    bufCopy.size = drawCommandsSize;

    vkCmdCopyBuffer(cmd, mInitialDrawCommandBuffer.mBuffer, mDrawCommandsBuffer.mBuffer, 1, &bufCopy);

    VkBufferMemoryBarrier barrier = makeBufferMemoryBarrier(
        mDrawCommandsBuffer.mBuffer, mVulkanResources->getGraphicQueue().mQueueFamilyIndex.value());
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 1,
        &barrier, 0, nullptr);
}

void ForwardPass::linkSceneData(const AllocatedBuffer& sceneBuffer)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, sceneBuffer.mBuffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (VkDescriptorSet set : mSceneDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void ForwardPass::linkLightData(const AllocatedBuffer& lightBuffer)
{
    DescriptorWriter writer;
    writer.writeBuffer(1, lightBuffer.mBuffer, sizeof(LightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (VkDescriptorSet set : mSceneDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void ForwardPass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
    Render::DescriptorWriter writer;
    writer.writeBuffer(0, objectBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (VkDescriptorSet set : mObjectDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void ForwardPass::draw()
{
    mRenderer->beginRender(true);

    mMeshBank->bindMeshBuffers(mRenderer->getCurrentCommandBuffer());

    const MaterialInstance& matInstance =
        mMaterialBank->getMaterialInstance(mMaterialBank->getDefaultMaterialInstanceId());

    vkCmdBindPipeline(
        mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, matInstance.mpBaseMaterial->mPipeline);

    //  bind scene and object data
    //  best to bind this before for all batches
    //  but now we need the pipeline layout so have to do it here
    //  optimize later
    vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
        matInstance.mpBaseMaterial->mPipelineLayout, 0, 1, &mSceneDescSets[mRenderer->getCurrentFrameIdx()], 0,
        nullptr);

    vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
        matInstance.mpBaseMaterial->mPipelineLayout, 1, 1, &mObjectDescSets[mRenderer->getCurrentFrameIdx()], 0,
        nullptr);

    vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
        matInstance.mpBaseMaterial->mPipelineLayout, 2, 1, &mDrawDataDescSets[mRenderer->getCurrentFrameIdx()], 0,
        nullptr);

    if (matInstance.mDescriptorSet != nullptr)
    {
        //  set 2 is material data, so start set is 2
        vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            matInstance.mpBaseMaterial->mPipelineLayout, 2, 1, &matInstance.mDescriptorSet, 0, nullptr);
    }

    vkCmdDrawIndexedIndirect(
        mRenderer->getCurrentCommandBuffer(), mDrawCommandsBuffer.mBuffer, 0, 1, sizeof(VkDrawIndexedIndirectCommand));

    mRenderer->finishRender();
}

void ForwardPass::cleanup()
{
    mVulkanResources->destroyBuffer(mInstanceObjectBuffer);
    mVulkanResources->destroyBuffer(mDrawCommandsBuffer);
    mVulkanResources->destroyBuffer(mInitialDrawCommandBuffer);
    mDrawCommandsData.clear();

    mDescriptorAllocator.destroyPools(mVulkanResources->getDevice());
}

const size_t ForwardPass::getDrawCommandBufferSize() const
{
    return sizeof(VkDrawIndexedIndirectCommand) * mDrawCommandsData.size();
}
} // namespace Bunny::Render
