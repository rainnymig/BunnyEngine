#include "ForwardPass.h"

#include "Material.h"
#include "MaterialBank.h"
#include "VulkanGraphicsRenderer.h"
#include "VulkanRenderResources.h"
#include "ShaderData.h"

#include <vulkan/vulkan.h>

#include <vector>

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

void ForwardPass::initializePass(VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout objectLayout)
{
    mSceneDescLayout = sceneLayout;
    mObjectDescLayout = objectLayout;

    //  create descriptor for object buffer and scene buffer
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .mRatio = 2},
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 6}
    };
    mDescriptorAllocator.init(mVulkanResources->getDevice(), 2, poolSizes);

    for (size_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mObjectDescLayout, &mObjectDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mSceneDescLayout, &mSceneDescSets[idx], 1, nullptr);
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
            //  the real instance count and first instance are the result of culling stage
            mDrawCommandsData.emplace_back(surface.mIndexCount, 0, surface.mFirstIndex, mesh.mVertexOffset, 0);
        }
    }

    const VkDeviceSize drawCommandsSize = mDrawCommandsData.size() * sizeof(VkDrawIndexedIndirectCommand);

    mDrawCommandsBuffer = mVulkanResources->createBuffer(drawCommandsSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    {
        void* mappedData = mDrawCommandsBuffer.mAllocationInfo.pMappedData;
        memcpy(mappedData, mDrawCommandsData.data(), drawCommandsSize);
    }
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
            mDrawCommandsData[idx].instanceCount = meshInstanceCounts.at(mesh.mId);
            mDrawCommandsData[idx].firstInstance = accumulatedInstances;
            idx++;
        }
        accumulatedInstances += meshInstanceCounts.at(mesh.mId);
    }

    const VkDeviceSize drawCommandsSize = mDrawCommandsData.size() * sizeof(VkDrawIndexedIndirectCommand);
    void* mappedData = mDrawCommandsBuffer.mAllocationInfo.pMappedData;
    memcpy(mappedData, mDrawCommandsData.data(), drawCommandsSize);
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

    // writer.updateSet(mVulkanResources->getDevice(), mObjectDescSets[mRenderer->getCurrentFrameIdx()]);
}

void ForwardPass::draw()
{
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

    if (matInstance.mDescriptorSet != nullptr)
    {
        //  set 2 is material data, so start set is 2
        vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            matInstance.mpBaseMaterial->mPipelineLayout, 2, 1, &matInstance.mDescriptorSet, 0, nullptr);
    }

    vkCmdDrawIndexedIndirect(
        mRenderer->getCurrentCommandBuffer(), mDrawCommandsBuffer.mBuffer, 0, 1, sizeof(VkDrawIndexedIndirectCommand));
}

void ForwardPass::cleanup()
{
    mVulkanResources->destroyBuffer(mDrawCommandsBuffer);
    mDrawCommandsData.clear();

    mDescriptorAllocator.destroyPools(mVulkanResources->getDevice());
}
} // namespace Bunny::Render
