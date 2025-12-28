#include "OceanPass.h"

#include "Descriptor.h"
#include "ErrorCheck.h"
#include "Error.h"
#include "Shader.h"
#include "GraphicsPipelineBuilder.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Helper.h"

#include <glm/gtx/euler_angles.hpp>

namespace Bunny::Render
{
OceanPass::OceanPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    std::string_view meshShaderPath, std::string_view fragShaderPath)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mMeshShaderPath(meshShaderPath),
      mFragShaderPath(fragShaderPath)
{
}

void OceanPass::draw() const
{

    VkCommandBuffer cmd = mRenderer->getCurrentCommandBuffer();
    const FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];

    //  wait for rendering from other passes onto the render target is done
    VkImageMemoryBarrier renderTargetBarrier = makeImageMemoryBarrier(frame.mRenderTarget->mImage,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_DEPENDENCY_BY_REGION_BIT, 0, nullptr, 0, nullptr, 1, &renderTargetBarrier);

    //  render
    std::vector<VkImageView> colorAttachmentViews;
    colorAttachmentViews.push_back(frame.mRenderTarget->mImageView);
    static constexpr bool updateDepth = true;
    static constexpr bool clearColor = false;
    mRenderer->beginRender(colorAttachmentViews, updateDepth, clearColor);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);

    vkCmdBindDescriptorSets(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout, 0, 2, &frame.mMeshDescSet, 0, nullptr);

    //  dispatch mesh pipeline
    vkCmdDrawMeshTasksEXT(cmd, GRID_SIZE / MESH_THREAD_COUNT_X, GRID_SIZE / MESH_THREAD_COUNT_Y, 1);

    mRenderer->finishRender();
}

void OceanPass::updateWorldParams(const glm::mat4& mvpMatrix, float elapsedTime, float deltaTime)
{
    mWorldParams.mMvpMatrix = mvpMatrix;
    mWorldParams.mElapsedTime = elapsedTime;
    mWorldParams.mDeltaTime = deltaTime;

    void* data = mWorldParamsBuffer.mAllocationInfo.pMappedData;
    memcpy(data, &mWorldParams, sizeof(WorldParams));
}

void OceanPass::linkLightAndCameraData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, lightData.mBuffer, sizeof(LightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeBuffer(1, cameraData.mBuffer, sizeof(PbrCameraData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mFragDescSet);
    }
}

void OceanPass::updateRenderTarget(const AllocatedImage* renderTarget)
{
    FrameData& frame = mFrameData[mRenderer->getCurrentFrameIdx()];
    frame.mRenderTarget = renderTarget;
}

BunnyResult OceanPass::initPipeline()
{
    VkDevice device = mVulkanResources->getDevice();

    Shader meshShader(mMeshShaderPath, device);
    Shader fragShader(mFragShaderPath, device);

    VkDescriptorSetLayout layouts[] = {mMeshDescLayout, mFragDescLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = layouts;
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &mPipelineLayout))
    mDeletionStack.AddFunction(
        [this]() { vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr); });

    GraphicsPipelineBuilder pipelineBuilder;
    pipelineBuilder.addShaderStage(meshShader.getShaderModule(), VK_SHADER_STAGE_MESH_BIT_EXT);
    pipelineBuilder.addShaderStage(fragShader.getShaderModule(), VK_SHADER_STAGE_FRAGMENT_BIT);
    pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineBuilder.setCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.setMultisamplingNone();
    pipelineBuilder.disableBlending(); //  opaque pipeline
    pipelineBuilder.enableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    std::vector<VkFormat> colorFormats{mRenderer->getSwapChainImageFormat()};
    pipelineBuilder.setColorAttachmentFormats(colorFormats);
    pipelineBuilder.setDepthFormat(mRenderer->getDepthImageFormat());
    pipelineBuilder.setPipelineLayout(mPipelineLayout);
    mPipeline = pipelineBuilder.build(device);
    if (mPipeline == nullptr)
    {
        return BUNNY_SAD;
    }

    mDeletionStack.AddFunction([this]() { vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult OceanPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts())

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .mRatio = 4},
    };
    mDescriptorAllocator.init(device, 4, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mMeshDescLayout, mFragDescLayout};
    for (FrameData& frame : mFrameData)
    {
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mMeshDescSet, 2);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult OceanPass::initDataAndResources()
{
    //  create wave field and world param data
    mWaveParams.mOctaves[0] = SineWaveOctave{
        .mK{0.5, 0},
        .mA{1},
        .mPhi{0}
    };
    mWaveParams.mOctaves[1] = SineWaveOctave{
        .mK{0.8f, 0},
        .mA{0.7},
        .mPhi{glm::pi<float>() / 4}
    };
    mWaveParams.mOctaves[2] = SineWaveOctave{
        .mK{1, 0},
        .mA{0.35},
        .mPhi{glm::pi<float>() / 8}
    };
    mWaveParams.mOctaves[3] = SineWaveOctave{
        .mK{1.5, 0},
        .mA{0.2},
        .mPhi{glm::pi<float>() / 2}
    };
    mWaveParams.mGridWidth = 0.1f;
    mWaveParams.mGridOrigin = glm::vec2(-mWaveParams.mGridWidth * GRID_SIZE / 2);

    //  create wave field and world buffers
    mVulkanResources->createBufferWithData(&mWaveParams, sizeof(WaveFieldParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO, mWaveParamsBuffer);
    mWorldParamsBuffer = mVulkanResources->createBuffer(sizeof(WorldParams), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);

    mDeletionStack.AddFunction([this]() {
        mVulkanResources->destroyBuffer(mWaveParamsBuffer);
        mVulkanResources->destroyBuffer(mWorldParamsBuffer);
    });

    //  update mesh desc set
    //  fragment desc set will be updated separately
    DescriptorWriter writer;
    writer.writeBuffer(0, mWaveParamsBuffer.mBuffer, sizeof(WaveFieldParams), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeBuffer(1, mWorldParamsBuffer.mBuffer, sizeof(WorldParams), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mMeshDescSet);
    }

    return BUNNY_HAPPY;
}

BunnyResult OceanPass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding descBinding{
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_MESH_BIT_EXT, nullptr};

    VkDevice device = mVulkanResources->getDevice();

    DescriptorLayoutBuilder builder;
    builder.addBinding(descBinding); //  wave field params
    descBinding.binding = 1;
    builder.addBinding(descBinding); //  world params
    mMeshDescLayout = builder.build(device);

    builder.clear();
    descBinding.binding = 0;
    descBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    builder.addBinding(descBinding);
    descBinding.binding = 1;
    builder.addBinding(descBinding);
    mFragDescLayout = builder.build(device);

    mDeletionStack.AddFunction([this]() {
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mMeshDescLayout, nullptr);
        vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mFragDescLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
