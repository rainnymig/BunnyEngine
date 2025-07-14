#include "RaytracingShadowPass.h"

#include "Error.h"
#include "ErrorCheck.h"
#include "MaterialBank.h"
#include "Shader.h"
#include "RaytracingPipelineBuilder.h"
#include "AlignHelpers.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Helper.h"

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

void RaytracingShadowPass::updateVertIdxBufferData(VkDeviceAddress vertBufAddress, VkDeviceAddress idxBufAddress)
{
    mVertIdxBufData.mVertexBufferAddress = vertBufAddress;
    mVertIdxBufData.mIndexBufferAddress = idxBufAddress;

    copyDataToBuffer(mVertIdxBufData, mVertIdxBufBuffer);
}

void RaytracingShadowPass::linkWorldData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData)
{
    DescriptorWriter writer;
    writer.writeBuffer(0, lightData.mBuffer, sizeof(PbrLightData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.writeBuffer(1, cameraData.mBuffer, sizeof(PbrCameraData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    for (const FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mWorldDescSet);
    }
}

void RaytracingShadowPass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
    Render::DescriptorWriter writer;
    writer.writeBuffer(0, objectBuffer.mBuffer, bufferSize, 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
    for (const FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mObjectDescSet);
    }
}

void RaytracingShadowPass::linkTopLevelAccelerationStructure(VkAccelerationStructureKHR acceStruct)
{
    DescriptorWriter writer;
    writer.writeAccelerationStructure(0, acceStruct);
    for (FrameData& frame : mFrameData)
    {
        writer.updateSet(mVulkanResources->getDevice(), frame.mRtDataDescSet);
    }
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

BunnyResult RaytracingShadowPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(buildRaytracingDescSetLayouts())

    //  allocate descriptor sets
    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             .mRatio = 4                                  },
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,             .mRatio = 6                                  },
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,     .mRatio = PbrMaterialBank::TEXTURE_ARRAY_SIZE},
        {.mType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, .mRatio = 2                                  },
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,              .mRatio = 2                                  }
    };
    mDescriptorAllocator.init(mVulkanResources->getDevice(), 12, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mMaterialBank->getWorldDescSetLayout(), mObjectDescSetLayout,
        mMaterialBank->getMaterialDescSetLayout(), mRtDataDescSetLayout};
    for (FrameData& frame : mFrameData)
    {
        //  allocate all 3 sets of one frame at once
        mDescriptorAllocator.allocate(mVulkanResources->getDevice(), descLayouts, &frame.mWorldDescSet, 4);

        //  link material data to material descriptor set
        mMaterialBank->updateMaterialDescriptorSet(frame.mMaterialDescSet);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult RaytracingShadowPass::initDataAndResources()
{
    //  create uniform buffer to hold vertex and index buffer device address
    mVertIdxBufBuffer =
        mVulkanResources->createBuffer(sizeof(VertexIndexBufferData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            VMA_MEMORY_USAGE_AUTO);
    mDeletionStack.AddFunction([this]() { mVulkanResources->destroyBuffer(mVertIdxBufBuffer); });

    //  create and bind out images for all frames
    VkExtent2D swapchainExtent = mRenderer->getSwapChainExtent();
    for (FrameData& frame : mFrameData)
    {
        frame.mOutImage = mVulkanResources->createImage(
            VkExtent3D{.width = swapchainExtent.width, .height = swapchainExtent.height, .depth = 1},
            VK_FORMAT_R32_UINT, VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
        mDeletionStack.AddFunction([this, &frame]() { mVulkanResources->destroyImage(frame.mOutImage); });

        DescriptorWriter writer;
        //  storage image, no need sampler?
        writer.writeImage(
            1, frame.mOutImage.mImageView, nullptr, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.updateSet(mVulkanResources->getDevice(), frame.mRtDataDescSet);

        writer.clear();
        writer.writeBuffer(
            1, mVertIdxBufBuffer.mBuffer, sizeof(VertexIndexBufferData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.updateSet(mVulkanResources->getDevice(), frame.mObjectDescSet);
    }
}

BunnyResult RaytracingShadowPass::buildPipelineLayout()
{
    //  take descriptor set layouts from material bank and build pipeline layout
    //  need to also add descriptor set layouts for vertex and index buffer as storage buffers
    std::array<VkDescriptorSetLayout, 4> descLayouts{mMaterialBank->getWorldDescSetLayout(), mObjectDescSetLayout,
        mMaterialBank->getMaterialDescSetLayout(), mRtDataDescSetLayout};
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
    //  build object descriptor set
    VkDescriptorSetLayoutBinding storageBufferBinding{
        0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr};
    VkDescriptorSetLayoutBinding addrUniformBinding{
        1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr};

    DescriptorLayoutBuilder builder;
    builder.addBinding(storageBufferBinding);
    builder.addBinding(addrUniformBinding);
    mObjectDescSetLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction([this]() {
        if (mObjectDescSetLayout != nullptr)
        {
            vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mObjectDescSetLayout, nullptr);
            mObjectDescSetLayout = nullptr;
        }
    });

    //  build descriptor set layouts that are specific to the ray tracing pipeline
    VkDescriptorSetLayoutBinding acceStructBinding{0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
        VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr};
    VkDescriptorSetLayoutBinding outImageBinding{
        1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr};

    builder.clear();
    builder.addBinding(acceStructBinding);
    builder.addBinding(outImageBinding);
    mRtDataDescSetLayout = builder.build(mVulkanResources->getDevice());

    mDeletionStack.AddFunction([this]() {
        if (mRtDataDescSetLayout != nullptr)
        {
            vkDestroyDescriptorSetLayout(mVulkanResources->getDevice(), mRtDataDescSetLayout, nullptr);
            mRtDataDescSetLayout = nullptr;
        }
    });

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
