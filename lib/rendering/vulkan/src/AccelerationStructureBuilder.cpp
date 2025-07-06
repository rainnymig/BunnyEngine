#include "AccelerationStructureBuilder.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "AlignHelpers.h"
#include "Error.h"
#include "Helper.h"

#include <algorithm>
#include <iterator>

namespace Bunny::Render
{
AccelerationStructureBuilder::AccelerationStructureBuilder(
    const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
    queryAcceStructProperties();
}

void AccelerationStructureBuilder::cleanup()
{
    for (BuiltAccelerationStructure& acceStruct : mBottomLevelAcceStructs)
    {
        destroyAcceStruct(acceStruct);
    }
    mBottomLevelAcceStructs.clear();

    if (mQueryPool != nullptr)
    {
        vkDestroyQueryPool(mVulkanResources->getDevice(), mQueryPool, nullptr);
        mQueryPool = nullptr;
    }
}

void AccelerationStructureBuilder::buildBottomLevelAccelerationStructures(
    const std::vector<AcceStructGeometryData>& blasData, VkBuildAccelerationStructureFlagsKHR flags)
{
    VkDeviceSize acceStructSize = 0;
    uint32_t blasCount = blasData.size();

    std::vector<AcceStructBuildData> acceStructBuildData;
    acceStructBuildData.reserve(blasCount);
    std::transform(blasData.begin(), blasData.end(), std::back_inserter(acceStructBuildData),
        [this, &blasData, flags](
            const AcceStructGeometryData& blasData) { return makeBottomLevelAcceStructBuildData(blasData, flags); });

    uint32_t minScratchBufAlignment = mAcceStructProperties.minAccelerationStructureScratchOffsetAlignment;

    //  scratch buffer to hold the staging data of the acceleration structure builder
    std::vector<VkDeviceAddress> scratchBufferAddress;
    AllocatedBuffer scratchBuffer =
        buildScratchBufferForAcceStruct(acceStructBuildData, minScratchBufAlignment, scratchBufferAddress);

    //  build and compact the acceleration structures
    buildAccelerationStructures(acceStructBuildData, scratchBuffer, scratchBufferAddress, mBottomLevelAcceStructs);
    compactAccelerationStructures(acceStructBuildData, mBottomLevelAcceStructs);

    mVulkanResources->destroyBuffer(scratchBuffer);
}

AccelerationStructureBuilder::AcceStructBuildData AccelerationStructureBuilder::makeBottomLevelAcceStructBuildData(
    const AcceStructGeometryData& blasData, VkBuildAccelerationStructureFlagsKHR flags) const
{
    AcceStructBuildData buildData{.mType = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .mGeometries = blasData.mGeometries,
        .mBuildRanges = blasData.mBuildRanges};

    buildData.mGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    buildData.mGeometryInfo.type = buildData.mType;
    buildData.mGeometryInfo.flags = blasData.mBuildFlags | flags;
    buildData.mGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildData.mGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
    buildData.mGeometryInfo.dstAccelerationStructure = VK_NULL_HANDLE;
    buildData.mGeometryInfo.geometryCount = static_cast<uint32_t>(buildData.mGeometries.size());
    buildData.mGeometryInfo.pGeometries = buildData.mGeometries.data();
    buildData.mGeometryInfo.ppGeometries = nullptr;
    buildData.mGeometryInfo.scratchData.deviceAddress = 0;

    size_t geometryCount = blasData.mBuildRanges.size();
    std::vector<uint32_t> primitiveCounts(geometryCount);
    for (size_t i = 0; i < geometryCount; ++i)
    {
        primitiveCounts[i] = blasData.mBuildRanges[i].primitiveCount;
    }

    vkGetAccelerationStructureBuildSizesKHR(mVulkanResources->getDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildData.mGeometryInfo, primitiveCounts.data(),
        &buildData.mSizeInfo);
}

AllocatedBuffer AccelerationStructureBuilder::buildScratchBufferForAcceStruct(
    const std::vector<AcceStructBuildData>& acceStructBuildData, uint32_t bufferAlignment,
    std::vector<VkDeviceAddress>& outScratchAddress) const
{
    //  max memory size for the scratch buffer
    //  256 MB
    //  currently we do not actually limit the size
    //  only trigger a warning if it's exceeded for later investigation
    static constexpr VkDeviceSize scratchBufferSizeLimit = 256 * 1024 * 1024;

    outScratchAddress.clear();
    outScratchAddress.reserve(acceStructBuildData.size());

    VkDeviceSize bufferSize = 0;

    for (const AcceStructBuildData& buildData : acceStructBuildData)
    {
        //  the buffer size at this point is also the offset in the scratch buffer for each acce struct
        outScratchAddress.push_back(bufferSize);
        VkDeviceSize alignedSize = Base::alignUp(buildData.mSizeInfo.buildScratchSize, bufferAlignment);
        bufferSize += alignedSize;
    }

    if (bufferSize > scratchBufferSizeLimit)
    {
        PRINT_WARNING(fmt::format("Acceleration structure build scratch buffer size {} is larger than the limit {}",
            bufferSize, scratchBufferSizeLimit));
    }

    AllocatedBuffer scratchBuffer = mVulkanResources->createBuffer(bufferSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        0, VMA_MEMORY_USAGE_AUTO); //  what vma creation flag needed?

    VkDeviceAddress scratchBufferAddress = mVulkanResources->getBufferDeviceAddress(scratchBuffer);
    for (VkDeviceAddress& address : outScratchAddress)
    {
        //  add the actual buffer device address to the offset
        address += scratchBufferAddress;
    }

    return scratchBuffer;
}

void AccelerationStructureBuilder::buildAccelerationStructures(std::vector<AcceStructBuildData>& acceStructBuildData,
    const AllocatedBuffer& scratchBuffer, const std::vector<VkDeviceAddress>& scratchAddress,
    std::vector<BuiltAccelerationStructure>& outAcceStructs)
{
    if (std::any_of(acceStructBuildData.begin(), acceStructBuildData.end(), [](const AcceStructBuildData& buildData) {
            return hasFlag(buildData.mGeometryInfo.flags, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR);
        }))
    {
        initializeQueryPool(static_cast<uint32_t>(acceStructBuildData.size()));
    }

    uint32_t structCount = static_cast<uint32_t>(acceStructBuildData.size());

    std::vector<VkAccelerationStructureKHR> vkAcceStructs;
    std::vector<VkAccelerationStructureBuildGeometryInfoKHR> geoInfos;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> rangeInfos;
    vkAcceStructs.reserve(structCount);
    geoInfos.reserve(structCount);
    rangeInfos.reserve(structCount);

    outAcceStructs.clear();
    outAcceStructs.reserve(structCount);

    for (size_t idx = 0; idx < structCount; idx++)
    {
        AcceStructBuildData& buildData = acceStructBuildData[idx];
        VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
        createInfo.type = buildData.mType;
        createInfo.size = buildData.mSizeInfo.accelerationStructureSize;

        //  create the acceleration structures (note: the acce structs are not built yet!)
        BuiltAccelerationStructure& builtStruct = outAcceStructs.emplace_back(createAcceStruct(createInfo));

        //  fill in the detail before actually building the acceleration structure
        buildData.mGeometryInfo.srcAccelerationStructure = VK_NULL_HANDLE;
        buildData.mGeometryInfo.dstAccelerationStructure = builtStruct.mAcceStruct;
        buildData.mGeometryInfo.scratchData.deviceAddress = scratchAddress[idx];
        buildData.mGeometryInfo.pGeometries = buildData.mGeometries.data();

        vkAcceStructs.push_back(builtStruct.mAcceStruct);
        geoInfos.push_back(buildData.mGeometryInfo);
        rangeInfos.push_back(buildData.mBuildRanges.data());
    }

    VkCommandBuffer cmd = mVulkanResources->startImmedidateCommand(VulkanRenderResources::CommandQueueType::Graphics);

    //  actually build the acceleration structures
    vkCmdBuildAccelerationStructuresKHR(cmd, structCount, geoInfos.data(), rangeInfos.data());

    //  wait for the acce structs finish building
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 1, &barrier, 0, nullptr, 0, nullptr);

    //  query the compact size of the built acceleration structures
    if (mQueryPool)
    {
        vkCmdWriteAccelerationStructuresPropertiesKHR(cmd, structCount, vkAcceStructs.data(),
            VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR, mQueryPool, 0);
    }

    mVulkanResources->endAndSubmitImmediateCommand(VulkanRenderResources::CommandQueueType::Graphics);
}

void AccelerationStructureBuilder::compactAccelerationStructures(
    std::vector<AcceStructBuildData>& acceStructBuildData, std::vector<BuiltAccelerationStructure>& acceStructs) const
{
    //  compact the already built acceleration structures
    //  this assumes the required compact size info is already queried and available in mQueryPool

    if (mQueryPool == VK_NULL_HANDLE)
    {
        return;
    }

    uint32_t structCount = acceStructs.size();

    //  retrieve the compacted sizes from the query pool.
    std::vector<VkDeviceSize> compactSizes(structCount);
    vkGetQueryPoolResults(mVulkanResources->getDevice(), mQueryPool, 0, structCount, structCount * sizeof(VkDeviceSize),
        compactSizes.data(), sizeof(VkDeviceSize), VK_QUERY_RESULT_WAIT_BIT);

    //  vector to hold the uncompacted acce structs for destruction later
    std::vector<BuiltAccelerationStructure> uncompactedAcceStructs;
    uncompactedAcceStructs.reserve(structCount);

    VkCommandBuffer cmd = mVulkanResources->startImmedidateCommand(VulkanRenderResources::CommandQueueType::Graphics);

    for (size_t idx = 0; idx < structCount; idx++)
    {
        VkDeviceSize compactSize = compactSizes[idx];
        if (compactSize > 0)
        {
            AcceStructBuildData& buildData = acceStructBuildData[idx];
            buildData.mSizeInfo.accelerationStructureSize = compactSize;
            uncompactedAcceStructs.push_back(acceStructs[idx]);

            //  create new BuiltAccelerationStructure to hold the compacted acce struct
            //  the newly created struct will replace the uncompacted one in the acceStructs vector
            VkAccelerationStructureCreateInfoKHR createInfo{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
            createInfo.size = compactSize;
            createInfo.type = buildData.mType;
            acceStructs[idx] = createAcceStruct(createInfo);

            //  copy the old acce struct to the new one
            VkCopyAccelerationStructureInfoKHR copyInfo{VK_STRUCTURE_TYPE_COPY_ACCELERATION_STRUCTURE_INFO_KHR};
            copyInfo.src = buildData.mGeometryInfo.dstAccelerationStructure;
            copyInfo.dst = acceStructs[idx].mAcceStruct;
            copyInfo.mode = VK_COPY_ACCELERATION_STRUCTURE_MODE_COMPACT_KHR;
            vkCmdCopyAccelerationStructureKHR(cmd, &copyInfo);

            buildData.mGeometryInfo.dstAccelerationStructure = acceStructs[idx].mAcceStruct;
        }
    }

    mVulkanResources->endAndSubmitImmediateCommand(VulkanRenderResources::CommandQueueType::Graphics);

    //  destroy all uncompacted acceleration structures
    for (BuiltAccelerationStructure& uncompactedStruct : uncompactedAcceStructs)
    {
        destroyAcceStruct(uncompactedStruct);
    }
}

BuiltAccelerationStructure AccelerationStructureBuilder::createAcceStruct(
    VkAccelerationStructureCreateInfoKHR createInfo) const
{
    BuiltAccelerationStructure builtAcceStruct;

    //  allocating the buffer to hold the acceleration structure
    builtAcceStruct.mBuffer = mVulkanResources->createBuffer(createInfo.size,
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 0,
        VMA_MEMORY_USAGE_AUTO);

    //  set the buffer in the create info to the one we just built and use the create info to create the acce struct
    createInfo.buffer = builtAcceStruct.mBuffer.mBuffer;
    vkCreateAccelerationStructureKHR(mVulkanResources->getDevice(), &createInfo, nullptr, &builtAcceStruct.mAcceStruct);

    //  get the device address of the acceleration structure
    VkAccelerationStructureDeviceAddressInfoKHR info{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    info.accelerationStructure = builtAcceStruct.mAcceStruct;
    builtAcceStruct.mAcceAddress = vkGetAccelerationStructureDeviceAddressKHR(mVulkanResources->getDevice(), &info);

    return builtAcceStruct;
}

void AccelerationStructureBuilder::destroyAcceStruct(BuiltAccelerationStructure& acceStruct) const
{
    if (acceStruct.mAcceStruct != nullptr)
    {
        vkDestroyAccelerationStructureKHR(mVulkanResources->getDevice(), acceStruct.mAcceStruct, nullptr);
        acceStruct.mAcceStruct = nullptr;
        acceStruct.mAcceAddress = 0;
    }

    mVulkanResources->destroyBuffer(acceStruct.mBuffer);
}

void AccelerationStructureBuilder::queryAcceStructProperties()
{
    mVulkanResources->getPhysicalDeviceProperties(&mAcceStructProperties);
}

void AccelerationStructureBuilder::initializeQueryPool(uint32_t queryCount)
{
    if (mQueryPool == nullptr)
    {
        VkQueryPoolCreateInfo info = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
        info.queryType = VK_QUERY_TYPE_ACCELERATION_STRUCTURE_COMPACTED_SIZE_KHR;
        info.queryCount = queryCount;
        vkCreateQueryPool(mVulkanResources->getDevice(), &info, nullptr, &mQueryPool);
    }

    //  reset the query pool to clear any old data or states.
    if (mQueryPool != nullptr)
    {
        vkResetQueryPool(mVulkanResources->getDevice(), mQueryPool, 0, queryCount);
    }
}
} // namespace Bunny::Render
