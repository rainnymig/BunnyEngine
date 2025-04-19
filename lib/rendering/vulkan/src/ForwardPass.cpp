#include "ForwardPass.h"

#include "Material.h"
#include "MaterialBank.h"
#include "VulkanGraphicsRenderer.h"
#include "VulkanRenderResources.h"
#include "ShaderData.h"

#include <vulkan/vulkan.h>

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

    for (size_t idx = 0; idx < mObjectDescSets.size(); idx++)
    {
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mObjectDescLayout, &mObjectDescSets[idx], 1, nullptr);
    }

    for (size_t idx = 0; idx < mObjectDescSets.size(); idx++)
    {
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mSceneDescLayout, &mSceneDescSets[idx], 1, nullptr);
    }
}

void ForwardPass::updateSceneData(const AllocatedBuffer& sceneBuffer)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, sceneBuffer.mBuffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (VkDescriptorSet set : mSceneDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void ForwardPass::updateLightData(const AllocatedBuffer& lightBuffer)
{
    DescriptorWriter writer;
    writer.writeBuffer(1, lightBuffer.mBuffer, sizeof(LightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (VkDescriptorSet set : mSceneDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void ForwardPass::updateObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
    Render::DescriptorWriter writer;
    writer.writeBuffer(0, objectBuffer.mBuffer, bufferSize, 0,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); //  storage buffer?
    for (VkDescriptorSet set : mObjectDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }

    // writer.updateSet(mVulkanResources->getDevice(), mObjectDescSets[mRenderer->getCurrentFrameIdx()]);
}

void ForwardPass::renderBatch(const RenderBatch& batch)
{

    //  bind vertex and index buffer
    //  to be moved to other places!
    mMeshBank->bindMeshBuffers(mRenderer->getCurrentCommandBuffer());

    const MaterialInstance& matInstance = mMaterialBank->getMaterialInstance(batch.mMaterialInstanceId);

    vkCmdBindPipeline(
        mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, matInstance.mpBaseMaterial->mPipeline);

    //  bind scene data
    //  best to bind this before for all batches
    //  but now we need the pipeline layout so have to do it here
    //  optimize later
    vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
        matInstance.mpBaseMaterial->mPipelineLayout, 0, 1, &mSceneDescSets[mRenderer->getCurrentFrameIdx()], 0,
        nullptr);

    //  bind object data
    //  set 1 is object data
    {
        Render::DescriptorWriter writer;
        writer.writeBuffer(0, batch.mObjectBuffer->mBuffer, batch.mInstanceCount * sizeof(ObjectData), 0,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER); //  storage buffer?
        writer.updateSet(mVulkanResources->getDevice(), mObjectDescSets[mRenderer->getCurrentFrameIdx()]);

        vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            matInstance.mpBaseMaterial->mPipelineLayout, 1, 1, &mObjectDescSets[mRenderer->getCurrentFrameIdx()], 0,
            nullptr);
    }

    if (matInstance.mDescriptorSet != nullptr)
    {
        //  set 2 is material data, so start set is 2
        vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
            matInstance.mpBaseMaterial->mPipelineLayout, 2, 1, &matInstance.mDescriptorSet, 0, nullptr);
    }

    //  draw indexed indirect
    //  temp
    //  should draw all meshes (surfaces) that has the same pipeline (material) and desc set (material instance)
    vkCmdDrawIndexedIndirect(mRenderer->getCurrentCommandBuffer(), mMeshBank->getDrawCommandsBuffer().mBuffer, 0, 1,
        sizeof(VkDrawIndexedIndirectCommand));

    //  temp
    //  this can be converted draw indirect
    // const MeshLite& meshToRender = mMeshBank->getMesh(batch.mMeshId);
    // for (const SurfaceLite& surface : meshToRender.mSurfaces)
    // {
    //     if (surface.mMaterialInstanceId == batch.mMaterialInstanceId)
    //     {
    //         vkCmdDrawIndexed(mRenderer->getCurrentCommandBuffer(), surface.mIndexCount, batch.mInstanceCount,
    //             surface.mFirstIndex, meshToRender.mVertexOffset, 0);
    //     }
    // }
}

void ForwardPass::renderAll()
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

    vkCmdDrawIndexedIndirect(mRenderer->getCurrentCommandBuffer(), mMeshBank->getDrawCommandsBuffer().mBuffer, 0, 1,
        sizeof(VkDrawIndexedIndirectCommand));
}

void ForwardPass::cleanup()
{
    mDescriptorAllocator.destroyPools(mVulkanResources->getDevice());
}
} // namespace Bunny::Render
