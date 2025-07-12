#include "RaytracingShadowPass.h"

#include "Error.h"
#include "ErrorCheck.h"
#include "MaterialBank.h"
#include "Shader.h"
#include "RaytracingPipelineBuilder.h"
#include "AlignHelpers.h"

#include <vulkan/vulkan.h>

#include <array>

namespace Bunny::Render
{
RaytracingShadowPass::RaytracingShadowPass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank)
    : super(vulkanResources, renderer, materialBank, meshBank)
{
    queryRaytracingProperties();
}

void RaytracingShadowPass::draw() const
{
}

BunnyResult RaytracingShadowPass::initPipeline()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildPipelineLayout())

    VkDevice device = mVulkanResources->getDevice();
    Shader rayGenShader("rtbasic_rgen.spv", device);
    Shader closestHitShader("rtshadow_chit.spv", device);
    Shader basicMissShader("rtbasic_miss.spv", device);
    Shader shadowMissShader("rtshadow_miss.spv", device);

    RaytracingPipelineBuilder builder;
    uint32_t rayGenIdx = builder.addShaderStage(rayGenShader.getShaderModule(), VK_SHADER_STAGE_RAYGEN_BIT_KHR);
    uint32_t basicMissIdx = builder.addShaderStage(basicMissShader.getShaderModule(), VK_SHADER_STAGE_MISS_BIT_KHR);
    uint32_t shadowMissIdx = builder.addShaderStage(shadowMissShader.getShaderModule(), VK_SHADER_STAGE_MISS_BIT_KHR);
    uint32_t closestHitIdx =
        builder.addShaderStage(closestHitShader.getShaderModule(), VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

    builder.addGeneralShaderGroup(rayGenIdx);
    builder.addGeneralShaderGroup(basicMissIdx);
    builder.addGeneralShaderGroup(shadowMissIdx);
    builder.addClosestHitShaderGroup(closestHitIdx);

    builder.setMaxRecursionDepth(2); //  1 camera ray + 1 shadow ray
                                     //  need more if multiple light?

    mPipeline = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction([this]() {
        if (mPipeline != nullptr)
        {
            vkDestroyPipeline(mVulkanResources->getDevice(), mPipeline, nullptr);
            mPipeline = nullptr;
        }
    });

    return BUNNY_HAPPY;
}

BunnyResult RaytracingShadowPass::buildPipelineLayout()
{
    //  take descriptor set layouts from material bank and build pipeline layout
    //  need to also add descriptor set layouts for vertex and index buffer as storage buffers
    std::array<VkDescriptorSetLayout, 3> descLayouts{mMaterialBank->getSceneDescSetLayout(),
        mMaterialBank->getObjectDescSetLayout(), mMaterialBank->getMaterialDescSetLayout()};
    VkPipelineLayoutCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    createInfo.pushConstantRangeCount = 0;
    createInfo.pPushConstantRanges = nullptr;
    createInfo.setLayoutCount = descLayouts.size();
    createInfo.pSetLayouts = descLayouts.data();

    VK_CHECK_OR_RETURN_BUNNY_SAD(
        vkCreatePipelineLayout(mVulkanResources->getDevice(), &createInfo, nullptr, &mPipelineLayout))

    mDeletionStack.AddFunction([this]() {
        if (mPipelineLayout != nullptr)
        {
            vkDestroyPipelineLayout(mVulkanResources->getDevice(), mPipelineLayout, nullptr);
            mPipelineLayout = nullptr;
        }
    });

    return BUNNY_HAPPY;
}

BunnyResult RaytracingShadowPass::buildRaytracingDescSetLayouts()
{
    //  build descriptor set layouts that are specific to the ray tracing pipeline
    //  such as desc sets for acceleration structures, vertex buffer, index buffer and output image

    return BUNNY_HAPPY;
}

BunnyResult Render::RaytracingShadowPass::buildShaderBindingTable()
{
    constexpr uint32_t rayGenShaderCount = 1;
    constexpr uint32_t missShaderCount = 2;
    constexpr uint32_t hitShaderCount = 1;
    constexpr uint32_t handleCount = rayGenShaderCount + missShaderCount + hitShaderCount;

    uint32_t handleSize = mRaytracingProperties.shaderGroupHandleSize;
    uint32_t handleAlignment = Base::alignUp(handleSize, mRaytracingProperties.shaderGroupHandleAlignment);
    uint32_t groupAlignment = mRaytracingProperties.shaderGroupBaseAlignment;

    //  The size member of pRayGenShaderBindingTable must be equal to its stride member
    mRayGenRegion.size = Base::alignUp(rayGenShaderCount * handleAlignment, groupAlignment);
    mRayGenRegion.stride = mRayGenRegion.size;
    mMissRegion.size = Base::alignUp(missShaderCount * handleAlignment, groupAlignment);
    mMissRegion.stride = handleAlignment;
    mHitRegion.size = Base::alignUp(hitShaderCount * handleAlignment, groupAlignment);
    mHitRegion.stride = handleAlignment;

    //  Get the shader group handles
    uint32_t dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);
    VK_CHECK_OR_RETURN_BUNNY_SAD(vkGetRayTracingShaderGroupHandlesKHR(
        mVulkanResources->getDevice(), mPipeline, 0, handleCount, dataSize, handles.data()))

    //  Allocate a buffer for storing the SBT.
    VkDeviceSize sbtSize = mRayGenRegion.size + mMissRegion.size + mHitRegion.size;
    mShaderBindingTableBuffer = mVulkanResources->createBuffer(sbtSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_AUTO);
    mDeletionStack.AddFunction([this]() { this->mVulkanResources->destroyBuffer(mShaderBindingTableBuffer); });

    VkDeviceAddress sbtAddress = mVulkanResources->getBufferDeviceAddress(mShaderBindingTableBuffer);
    mRayGenRegion.deviceAddress = sbtAddress;
    mMissRegion.deviceAddress = mRayGenRegion.deviceAddress + mRayGenRegion.size;
    mHitRegion.deviceAddress = mMissRegion.deviceAddress + mMissRegion.size;

    //  Helper to retrieve the handle data
    auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

    uint8_t* mappedSbt = reinterpret_cast<uint8_t*>(mShaderBindingTableBuffer.mAllocationInfo.pMappedData);
    uint8_t* dst = mappedSbt;
    uint32_t handleIdx{0};
    // Raygen
    dst = mappedSbt;
    memcpy(dst, getHandle(handleIdx++), handleSize);
    // Miss
    dst = mappedSbt + mRayGenRegion.size;
    for (uint32_t c = 0; c < missShaderCount; c++)
    {
        memcpy(dst, getHandle(handleIdx++), handleSize);
        dst += mMissRegion.stride;
    }
    // Hit
    dst = mappedSbt + mRayGenRegion.size + mMissRegion.size;
    for (uint32_t c = 0; c < hitShaderCount; c++)
    {
        memcpy(dst, getHandle(handleIdx++), handleSize);
        dst += mHitRegion.stride;
    }

    return BUNNY_HAPPY;
}

void RaytracingShadowPass::queryRaytracingProperties()
{
    mVulkanResources->getPhysicalDeviceProperties(&mRaytracingProperties);
}

} // namespace Bunny::Render
