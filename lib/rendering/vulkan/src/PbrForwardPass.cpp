#include "PbrForwardPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "MaterialBank.h"
#include "MeshBank.h"
#include "Shader.h"
#include "ShaderData.h"
#include "GraphicsPipelineBuilder.h"
#include "Helper.h"

namespace Bunny::Render
{
PbrForwardPass::PbrForwardPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank, std::string_view vertShader,
    std::string_view fragShader)
    : super(vulkanResources, renderer, materialBank, meshBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void PbrForwardPass::draw() const
{
    //  only draw opaque surfaces, skip if none
    if (mMeshBank->getOpaqueSurfaceCount() == 0)
    {
        return;
    }

    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    //  transition the render target image layout back to color attachment optimal
    VkImageMemoryBarrier renderTargetBarrier = makeImageMemoryBarrier(frame.mSceneRenderTarget->mImage,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &renderTargetBarrier);

    std::vector<VkImageView> colorAttachmentViews;
    colorAttachmentViews.push_back(frame.mSceneRenderTarget->mImageView);
    auto renderHelper = mRenderer->getRenderHelper()
                            .setColorAttachments(colorAttachmentViews)
                            .setClearColor(true)
                            .setClearDepth(true)
                            .setUpdateDepth(true)
                            .setMultiSample(mRenderer->isMultiSampleEnabled(), false)
                            .beginRender();

    //  bind mesh vertex and index buffers
    mMeshBank->bindMeshBuffers(cmd);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    //  bind all scene, object and material descriptor sets at once
    //  they are laid out properly in FrameData
    //  if the layout changes this needs to be updated
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 4,
        &mFrameData[mRenderer->getCurrentFrameIdx()].mWorldDescSet, 0, nullptr);

    //  only draw the opaque surfaces
    vkCmdDrawIndexedIndirect(
        cmd, mDrawCommandsBuffer.mBuffer, 0, mMeshBank->getOpaqueSurfaceCount(), sizeof(VkDrawIndexedIndirectCommand));

    renderHelper.finishRender();
}

void PbrForwardPass::buildDrawCommands()
{
    const std::vector<MeshLite>& meshes = mMeshBank->getMeshes();
    mDrawCommandsData.clear();
    mSurfaceToCommandMapData.clear();

    //  build the draw indirect commands
    //  the commands for all the opaque surfaces will be in front
    //  and the commands for all the transparent surfaces will be in the back
    //  this way we can easily draw the opaque and transparent surfaces with two draw calls
    //  at the same time a buffer for a mapping from surface to command is built
    //  so that in the culling pass the instance count of a surface
    //  can be correctly updated in its corresponding draw command

    //  add a draw indirect command for each opaque surface in the mesh
    for (const MeshLite& mesh : meshes)
    {
        for (const SurfaceLite& surface : mesh.mSurfaces)
        {
            uint32_t& sufToComIdx = mSurfaceToCommandMapData.emplace_back();
            if (surface.mTransparency == SurfaceTransparency::Opaque)
            {
                sufToComIdx = mDrawCommandsData.size();
                mDrawCommandsData.emplace_back(surface.mIndexCount, 0, surface.mFirstIndex, surface.mVertexOffset, 0);
            }
        }
    }

    //  add a draw indirect command for each transparent surface in the mesh
    uint32_t surfaceIdx = 0;
    for (const MeshLite& mesh : meshes)
    {
        for (const SurfaceLite& surface : mesh.mSurfaces)
        {
            if (surface.mTransparency == SurfaceTransparency::Transparent)
            {
                mSurfaceToCommandMapData[surfaceIdx] = mDrawCommandsData.size();
                mDrawCommandsData.emplace_back(surface.mIndexCount, 0, surface.mFirstIndex, surface.mVertexOffset, 0);
            }
            surfaceIdx++;
        }
    }

    const VkDeviceSize drawCommandsSize = getContainerDataSize(mDrawCommandsData);

    mInitialDrawCommandBuffer = mVulkanResources->createBuffer(drawCommandsSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    mDrawCommandsBuffer = mVulkanResources->createBuffer(drawCommandsSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_MEMORY_USAGE_AUTO);

    mVulkanResources->createBufferWithData(mSurfaceToCommandMapData.data(),
        getContainerDataSize(mSurfaceToCommandMapData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT, VMA_MEMORY_USAGE_AUTO, mSurfaceToCommandMapBuffer);

    mDeletionStack.AddFunction([this]() {
        mVulkanResources->destroyBuffer(mDrawCommandsBuffer);
        mVulkanResources->destroyBuffer(mInitialDrawCommandBuffer);
        mVulkanResources->destroyBuffer(mInstanceObjectBuffer);
        mVulkanResources->destroyBuffer(mSurfaceToCommandMapBuffer);
        mDrawCommandsData.clear();
    });
}

void PbrForwardPass::updateDrawInstanceCounts(std::unordered_map<IdType, size_t> meshInstanceCounts)
{
    //  possible optimization:
    //  set the instance counts to a fixed value (2^n)
    //  if the meshInstanceCounts does not exceed this value then no need to update the buffers

    const std::vector<MeshLite>& meshes = mMeshBank->getMeshes();

    size_t surfaceIdx = 0;
    size_t accumulatedInstances = 0;
    for (const MeshLite& mesh : meshes)
    {
        for (const SurfaceLite& surface : mesh.mSurfaces)
        {
            size_t commandIdx = mSurfaceToCommandMapData.at(surfaceIdx);
            mDrawCommandsData[commandIdx].firstInstance = accumulatedInstances;
            surfaceIdx++;
        }
        accumulatedInstances += meshInstanceCounts.at(mesh.mId);
    }

    const VkDeviceSize drawCommandsSize = getContainerDataSize(mDrawCommandsData);
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
        writer.writeBuffer(1, mInstanceObjectBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
        for (FrameData& frame : mFrameData)
        {
            writer.updateSet(mVulkanResources->getDevice(), frame.mObjectDescSet);
        }
    }
}

void PbrForwardPass::prepareDrawCommandsForFrame()
{
    const VkDeviceSize drawCommandsSize = getContainerDataSize(mDrawCommandsData);
    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    mVulkanResources->copyBuffer(cmd, mInitialDrawCommandBuffer.mBuffer, mDrawCommandsBuffer.mBuffer, drawCommandsSize);
    mVulkanResources->transitionBufferAccess(cmd, mDrawCommandsBuffer.mBuffer, VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void PbrForwardPass::linkWorldData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, lightData.mBuffer, sizeof(PbrLightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeBuffer(1, cameraData.mBuffer, sizeof(PbrCameraData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (const FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mWorldDescSet);
    }
}

void PbrForwardPass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
    Render::DescriptorWriter writer;
    writer.writeBuffer(0, objectBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (const FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mObjectDescSet);
    }
}

void PbrForwardPass::linkShadowData(std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> shadowImageViews)
{
    Render::DescriptorWriter writer;
    VkDevice device = mVulkanResources->getDevice();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        const FrameData& frame = mFrameData[i];
        writer.clear();
        writer.writeImage(0, shadowImageViews[i], nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(device, frame.mEffectDescSet);
    }
}

void PbrForwardPass::updateRenderTarget(const AllocatedImage* renderTarget)
{
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    frame.mSceneRenderTarget = renderTarget;
}

const size_t PbrForwardPass::getDrawCommandBufferSize() const
{
    return getContainerDataSize(mDrawCommandsData);
}

BunnyResult PbrForwardPass::initPipeline()
{
    VkDevice device = mVulkanResources->getDevice();

    //  load shader file
    Shader vertexShader(mVertexShaderPath, device);
    Shader fragmentShader(mFragmentShaderPath, device);

    mPipelineLayout = mMaterialBank->getPbrForwardPipelineLayout();

    //  vertex info
    auto bindingDescription = getBindingDescription<NormalVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = NormalVertex::getAttributeDescriptions();

    //  build pipeline
    GraphicsPipelineBuilder builder;
    builder.addShaderStage(vertexShader.getShaderModule(), VK_SHADER_STAGE_VERTEX_BIT);
    builder.addShaderStage(fragmentShader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT);
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    if (mRenderer->isMultiSampleEnabled())
    {
        builder.setMultiSamplingCount(mRenderer->getRenderMultiSampleCount());
    }
    else
    {
        builder.setMultisamplingNone();
    }
    builder.disableBlending(); //  opaque pipeline
    builder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    std::vector<VkFormat> colorFormats{mRenderer->getSwapChainImageFormat()};
    builder.setColorAttachmentFormats(colorFormats);
    builder.setDepthFormat(mRenderer->getDepthImageFormat());
    builder.setPipelineLayout(mPipelineLayout);

    mPipeline = builder.build(device);

    mDeletionStack.AddFunction([this]() { vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult PbrForwardPass::initDescriptors()
{
    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         .mRatio = 4                                  },
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 4                                  },
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = PbrMaterialBank::TEXTURE_ARRAY_SIZE},
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          .mRatio = 2                                  },
    };
    mDescriptorAllocator.init(device, 10, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mMaterialBank->getWorldDescSetLayout(),
        mMaterialBank->getObjectDescSetLayout(), mMaterialBank->getMaterialDescSetLayout(),
        mMaterialBank->getEffectDescSetLayout()};
    for (FrameData& frame : mFrameData)
    {
        //  allocate all 4 sets of one frame at once
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mWorldDescSet, 4);

        //  link material data to material descriptor set
        mMaterialBank->updateMaterialDescriptorSet(frame.mMaterialDescSet, mMeshBank);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });
    return BUNNY_HAPPY;
}

BunnyResult PbrForwardPass::initDataAndResources()
{
    return BUNNY_HAPPY;
}
} // namespace Bunny::Render
