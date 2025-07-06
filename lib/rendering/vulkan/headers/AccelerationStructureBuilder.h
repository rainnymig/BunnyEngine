#pragma once

#include "AccelerationStructureData.h"
#include "Fundamentals.h"

#include <vulkan/vulkan.h>
#include <glm/mat4x4.hpp>

#include <vector>
#include <algorithm>
#include <iterator>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;

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

    BuiltAccelerationStructure createAcceStruct(VkAccelerationStructureCreateInfoKHR createInfo) const;
    void destroyAcceStruct(BuiltAccelerationStructure& acceStruct) const;

    void queryAcceStructProperties();
    void initializeQueryPool(uint32_t queryCount);

    VkPhysicalDeviceAccelerationStructurePropertiesKHR mAcceStructProperties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};

    std::vector<BuiltAccelerationStructure> mBottomLevelAcceStructs;

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
} // namespace Bunny::Render
