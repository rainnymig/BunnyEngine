#include "AccelerationStructureBuilder.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "AlignHelpers.h"
#include "Error.h"

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

void AccelerationStructureBuilder::queryAcceStructProperties()
{
    mVulkanResources->getPhysicalDeviceProperties(&mAcceStructProperties);
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

} // namespace Bunny::Render
