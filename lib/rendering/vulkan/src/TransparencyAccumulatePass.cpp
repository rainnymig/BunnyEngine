#include "TransparencyAccumulatePass.h"

#include "MaterialBank.h"
#include "Vertex.h"
#include "GraphicsPipelineBuilder.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Helper.h"

namespace Bunny::Render
{
Render::TransparencyAccumulatePass::TransparencyAccumulatePass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
    std::string_view vertShader, std::string_view fragShader)
    : super(vulkanResources, renderer, materialBank, meshBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void Render::TransparencyAccumulatePass::draw() const
{
}

void Render::TransparencyAccumulatePass::linkWorldData(
    const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData)
{
}

void Render::TransparencyAccumulatePass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
}

void Render::TransparencyAccumulatePass::linkShadowData(std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> shadowImageViews)
{
}

BunnyResult Render::TransparencyAccumulatePass::initPipeline()
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

    builder.addColorAttachmentWithBlend(VK_FORMAT_R32G32B32A32_SFLOAT,
        makePipelineColorBlendAttachmentState(VK_BLEND_FACTOR_ONE, VK_BLEND_FACTOR_ONE)); //  accumulate
    builder.addColorAttachmentWithBlend(VK_FORMAT_R16_SFLOAT, //  maybe R8_UNORM or R8_SNORM is enough?
        makePipelineColorBlendAttachmentState(VK_BLEND_FACTOR_ZERO, VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR)); //  revealage

    //  depth test is enabled so that we don't render transparent objects behind opaque ones
    //  but depth write is disabled so that transparent objects don't block each other
    builder.enableDepthTest(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
    builder.setDepthFormat(mRenderer->getDepthImageFormat());
    builder.setPipelineLayout(mPipelineLayout);

    mPipeline = builder.build(device);

    mDeletionStack.AddFunction([this]() { vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult Render::TransparencyAccumulatePass::initDescriptors()
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

BunnyResult Render::TransparencyAccumulatePass::initDataAndResources()
{
    //  create the render targets

    //  accumulate

    //  revealage

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
