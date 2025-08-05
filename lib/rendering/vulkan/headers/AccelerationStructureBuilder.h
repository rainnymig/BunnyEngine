#pragma once

#include "AccelerationStructureData.h"
#include "Fundamentals.h"
#include "ErrorCheck.h"
#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"
#include "Helper.h"

#include <volk.h>
#include <glm/mat4x4.hpp>

#include <vector>
#include <algorithm>
#include <iterator>
#include <cassert>

namespace Bunny::Render
{

class AccelerationStructureBuilder
{
  public:
    AccelerationStructureBuilder(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    void cleanup();

    void buildBottomLevelAccelerationStructures(
        const std::vector<AcceStructGeometryData>& blasData, VkBuildAccelerationStructureFlagsKHR flags);

    template <typename ObjectDataType>
    void buildTopLevelAccelerationStructures(
        const std::vector<ObjectDataType>& objectData, VkBuildAccelerationStructureFlagsKHR flags);

    const BuiltAccelerationStructure& getTopLevelAccelerationStructure() const { return mTopLevelAcceStruct; }

  private:
    struct AcceStructBuildData
    {
        VkAccelerationStructureTypeKHR mType = VK_ACCELERATION_STRUCTURE_TYPE_MAX_ENUM_KHR;
        std::vector<VkAccelerationStructureGeometryKHR> mGeometries;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> mBuildRanges;
        VkAccelerationStructureBuildGeometryInfoKHR mGeometryInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
        VkAccelerationStructureBuildSizesInfoKHR mSizeInfo{
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    };

    AcceStructBuildData makeBottomLevelAcceStructBuildData(
        const AcceStructGeometryData& blasData, VkBuildAccelerationStructureFlagsKHR flags) const;
    //  build scratch buffer for building acceleration structure
    //  it builds 1 scratch buffer for all acce structs
    //  outScratchAddress vector contains the device address for each acce struct in the scratch buffer
    AllocatedBuffer buildScratchBufferForAcceStruct(const std::vector<AcceStructBuildData>& acceStructBuildData,
        uint32_t bufferAlignment, std::vector<VkDeviceAddress>& outScratchAddress) const;
    void buildAccelerationStructures(std::vector<AcceStructBuildData>& acceStructBuildData,
        const AllocatedBuffer& scratchBuffer, const std::vector<VkDeviceAddress>& scratchAddress,
        std::vector<BuiltAccelerationStructure>& outAcceStructs);
    void compactAccelerationStructures(std::vector<AcceStructBuildData>& acceStructBuildData,
        std::vector<BuiltAccelerationStructure>& acceStructs) const;

    template <typename ObjectDataType>
    VkAccelerationStructureInstanceKHR makeAcceStructInstance(const ObjectDataType& data) const;
    template <typename InstanceType>
    void buildTopLevelAcceStructFromInstances(
        const std::vector<InstanceType>& instances, VkBuildAccelerationStructureFlagsKHR flags, bool isUpdate);

    void prepareAcceBuildGeoSizeInfo(AcceStructBuildData& buildData, VkBuildAccelerationStructureFlagsKHR flags) const;

    BuiltAccelerationStructure createAcceStruct(VkAccelerationStructureCreateInfoKHR createInfo) const;
    void destroyAcceStruct(BuiltAccelerationStructure& acceStruct) const;
    void updateAcceStruct(
        AcceStructBuildData& buildData, BuiltAccelerationStructure& acceStruct, VkDeviceAddress scratchAddress);

    void queryAcceStructProperties();
    void initializeQueryPool(uint32_t queryCount);

    VkPhysicalDeviceAccelerationStructurePropertiesKHR mAcceStructProperties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};

    std::vector<BuiltAccelerationStructure> mBottomLevelAcceStructs;
    BuiltAccelerationStructure mTopLevelAcceStruct;

    //  query pool for query acceleration structure size for compaction
    VkQueryPool mQueryPool = VK_NULL_HANDLE;

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
};

template <typename ObjectDataType>
inline void AccelerationStructureBuilder::buildTopLevelAccelerationStructures(
    const std::vector<ObjectDataType>& objectData, VkBuildAccelerationStructureFlagsKHR flags)
{
    std::vector<VkAccelerationStructureInstanceKHR> acceStructInstances;
    acceStructInstances.reserve(objectData.size());
    std::transform(objectData.begin(), objectData.end(), std::back_inserter(acceStructInstances),
        [this](const ObjectDataType& data) { return makeAcceStructInstance(data); });
    constexpr bool isUpdate = false;
    buildTopLevelAcceStructFromInstances<VkAccelerationStructureInstanceKHR>(acceStructInstances, flags, isUpdate);
}

template <typename ObjectDataType>
inline VkAccelerationStructureInstanceKHR AccelerationStructureBuilder::makeAcceStructInstance(
    const ObjectDataType& data) const
{
    //  this should run after the bottom level acceleration structures are built

    VkAccelerationStructureInstanceKHR instance;
    instance.transform = convertToVkTransformMatrix(data.model);
    instance.instanceCustomIndex = data.meshId; //  this is used to reference the bottom level acce struct
    instance.accelerationStructureReference =
        mBottomLevelAcceStructs[data.meshId].mAcceAddress; // device address of the blas
    instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.mask = 0xff; //  an instance is hit only when rayMask & instance.mask != 0
                          //  setting to 0xff meaning this instance can be hit by all rays
    instance.instanceShaderBindingTableRecordOffset = 0;

    return instance;
}

template <typename InstanceType>
inline void AccelerationStructureBuilder::buildTopLevelAcceStructFromInstances(
    const std::vector<InstanceType>& instances, VkBuildAccelerationStructureFlagsKHR flags, bool isUpdate)
{
    assert(mTopLevelAcceStruct.mAcceStruct == VK_NULL_HANDLE || isUpdate);
    uint32_t instanceCount = static_cast<uint32_t>(instances.size());

    //  create the buffer containing the instance data
    VkDeviceSize instanceBufferSize = sizeof(InstanceType) * instances.size();
    AllocatedBuffer instanceBuffer;
    mVulkanResources->createBufferWithData(instances.data(), instanceBufferSize,
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
        0, VMA_MEMORY_USAGE_AUTO, instanceBuffer);
    VkDeviceAddress instanceBufferAddress = mVulkanResources->getBufferDeviceAddress(instanceBuffer);

    //  create geometry instance data from the instance buffer for building the tlas
    VkAccelerationStructureGeometryInstancesDataKHR geometryInstances{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    geometryInstances.data.deviceAddress = instanceBufferAddress;

    VkAccelerationStructureGeometryKHR geometry{VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances = geometryInstances;

    VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
    rangeInfo.primitiveCount = static_cast<uint32_t>(instanceCount);

    //  prepare acceleration struct build data from the geometry instances data
    std::vector<AcceStructBuildData> acceStructBuildData;
    AcceStructBuildData& buildData = acceStructBuildData.emplace_back();
    buildData.mType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    buildData.mGeometries.push_back(geometry);
    buildData.mBuildRanges.push_back(rangeInfo);
    prepareAcceBuildGeoSizeInfo(buildData, flags);

    //  allocate scratch buffer for building
    VkDeviceSize scratchSize = isUpdate ? buildData.mSizeInfo.updateScratchSize : buildData.mSizeInfo.buildScratchSize;
    AllocatedBuffer scratchBuffer = mVulkanResources->createBuffer(scratchSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, 0, VMA_MEMORY_USAGE_AUTO);
    VkDeviceAddress scratchAddress = mVulkanResources->getBufferDeviceAddress(scratchBuffer);

    if (isUpdate)
    {
        updateAcceStruct(buildData, mTopLevelAcceStruct, scratchAddress);
    }
    else
    {
        std::vector<VkDeviceAddress> scratchAddresses;
        scratchAddresses.push_back(scratchAddress);

        //  temp vector to hold the built acce struct
        //  because buildAccelerationStructures() accepts an vector of built acce structs as param
        //  may optimize later
        std::vector<BuiltAccelerationStructure> builtAcceStruct;
        buildAccelerationStructures(acceStructBuildData, scratchBuffer, scratchAddresses, builtAcceStruct);
        mTopLevelAcceStruct = builtAcceStruct[0];
    }

    mVulkanResources->destroyBuffer(scratchBuffer);
    mVulkanResources->destroyBuffer(instanceBuffer);
}

} // namespace Bunny::Render
