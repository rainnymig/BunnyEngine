#include "GBufferPass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Shader.h"
#include "ShaderData.h"
#include "GraphicsPipelineBuilder.h"
#include "Helper.h"

namespace Bunny::Render
{
GBufferPass::GBufferPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const MeshBank<NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer),
      mMeshBank(meshBank)
{
}

void GBufferPass::initializePass()
{
    initDescriptorSets();
    initPipeline();
    createImages();
}

void GBufferPass::buildDrawCommands()
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

void GBufferPass::updateDrawInstanceCounts(std::unordered_map<IdType, size_t> meshInstanceCounts)
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

void GBufferPass::resetDrawCommands()
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

void GBufferPass::linkSceneData(const AllocatedBuffer& sceneBuffer)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, sceneBuffer.mBuffer, sizeof(SceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (VkDescriptorSet set : mSceneDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void GBufferPass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
    Render::DescriptorWriter writer;
    writer.writeBuffer(0, objectBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (VkDescriptorSet set : mObjectDescSets)
    {
        writer.updateSet(mVulkanResources->getDevice(), set);
    }
}

void GBufferPass::draw()
{
    //  setup render attachments

    mMeshBank->bindMeshBuffers(mRenderer->getCurrentCommandBuffer());

    vkCmdBindPipeline(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    //  bind scene and object data
    //  best to bind this before for all batches
    //  but now we need the pipeline layout so have to do it here
    //  optimize later
    vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0,
        1, &mSceneDescSets[mRenderer->getCurrentFrameIdx()], 0, nullptr);

    vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 1,
        1, &mObjectDescSets[mRenderer->getCurrentFrameIdx()], 0, nullptr);

    vkCmdBindDescriptorSets(mRenderer->getCurrentCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 2,
        1, &mDrawDataDescSets[mRenderer->getCurrentFrameIdx()], 0, nullptr);

    vkCmdDrawIndexedIndirect(
        mRenderer->getCurrentCommandBuffer(), mDrawCommandsBuffer.mBuffer, 0, 1, sizeof(VkDrawIndexedIndirectCommand));
}

void GBufferPass::cleanup()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        mVulkanResources->destroyImage(mColorMaps[i]);
        mVulkanResources->destroyImage(mFragPosMaps[i]);
        mVulkanResources->destroyImage(mNormalTexCoordMaps[i]);
    }

    mVulkanResources->destroyBuffer(mInstanceObjectBuffer);
    mVulkanResources->destroyBuffer(mDrawCommandsBuffer);
    mVulkanResources->destroyBuffer(mInitialDrawCommandBuffer);
    mDrawCommandsData.clear();

    mDescriptorAllocator.destroyPools(mVulkanResources->getDevice());

    vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr);
    vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr);
}

void GBufferPass::initDescriptorSets()
{
    VkDevice device = mVulkanResources->getDevice();

    //  build descriptor set layouts
    DescriptorLayoutBuilder layoutBuilder;
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        layoutBuilder.addBinding(uniformBufferLayout);
    }
    mStorageDescLayout = layoutBuilder.build(device);

    layoutBuilder.clear();
    {
        VkDescriptorSetLayoutBinding uniformBufferLayout{};
        uniformBufferLayout.binding = 0;
        uniformBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBufferLayout.descriptorCount = 1;
        uniformBufferLayout.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        uniformBufferLayout.pImmutableSamplers = nullptr;
        layoutBuilder.addBinding(uniformBufferLayout);
    }
    mUniformDescLayout = layoutBuilder.build(device);

    //  init descriptor allocator
    //  create descriptor for object buffer and scene buffer
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .mRatio = 4},
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 6}
    };
    mDescriptorAllocator.init(mVulkanResources->getDevice(), 4, poolSizes);

    //  allocator descriptor sets
    for (size_t idx = 0; idx < MAX_FRAMES_IN_FLIGHT; idx++)
    {
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mUniformDescLayout, &mSceneDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mStorageDescLayout, &mObjectDescSets[idx], 1, nullptr);
        mDescriptorAllocator.allocate(
            mVulkanResources->getDevice(), &mStorageDescLayout, &mDrawDataDescSets[idx], 1, nullptr);
    }
}

BunnyResult GBufferPass::initPipeline()
{
    //  load shader
    Shader vertexShader(mVertexShaderPath, mVulkanResources->getDevice());
    Shader fragmentShader(mFragmentShaderPath, mVulkanResources->getDevice());

    //  build pipeline layout
    VkDescriptorSetLayout layouts[] = {mUniformDescLayout, mStorageDescLayout, mStorageDescLayout};

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 3;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    vkCreatePipelineLayout(mVulkanResources->getDevice(), &pipelineLayoutInfo, nullptr, &mPipelineLayout);

    //  vertex info
    auto bindingDescription = getBindingDescription<NormalVertex>(0, VertexInputRate::Vertex);
    auto attributeDescriptions = NormalVertex::getAttributeDescriptions();

    //  build pipeline
    GraphicsPipelineBuilder builder;
    builder.setShaders(vertexShader.getShaderModule(), fragmentShader.getShaderModule());
    builder.setVertexInput(attributeDescriptions.data(), attributeDescriptions.size(), &bindingDescription, 1);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    builder.setMultisamplingNone();
    builder.disableBlending(); //  opaque pipeline
    builder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    std::vector<VkFormat> colorFormats{VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R16G16B16A16_SFLOAT,
        VK_FORMAT_R16G16B16A16_SFLOAT}; //  color, frag pos, normal texcoord
    builder.setColorAttachmentFormats(colorFormats);
    builder.setDepthFormat(mRenderer->getDepthImageFormat());
    builder.setPipelineLayout(mPipelineLayout);

    mPipeline = builder.build(mVulkanResources->getDevice());

    return BUNNY_HAPPY;
}

void GBufferPass::createImages()
{
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        VkExtent2D swapChainExtent = mRenderer->getSwapChainExtent();
        VkExtent3D imageExtent{.width = swapChainExtent.width, .height = swapChainExtent.height, .depth = 1};
        mColorMaps[i] = mVulkanResources->createImage(imageExtent, VK_FORMAT_R8G8B8A8_UNORM,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED);
        mFragPosMaps[i] = mVulkanResources->createImage(imageExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED);
        mNormalTexCoordMaps[i] = mVulkanResources->createImage(imageExtent, VK_FORMAT_R16G16B16A16_SFLOAT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED);
    }
}

} // namespace Bunny::Render
