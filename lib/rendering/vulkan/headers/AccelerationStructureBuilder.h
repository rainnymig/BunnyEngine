#pragma once

#include "AccelerationStructureData.h"
#include "Fundamentals.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;

class AccelerationStructureBuilder
{
  public:
    AccelerationStructureBuilder(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    void buildBottomLevelAccelerationStructures(
        const std::vector<AcceStructGeometryData>& blasData, VkBuildAccelerationStructureFlagsKHR flags);

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
    void queryAcceStructProperties();

    //  build scratch buffer for building acceleration structure
    //  it builds 1 scratch buffer for all acce structs
    //  outScratchAddress vector contains the device address for each acce struct in the scratch buffer
    AllocatedBuffer buildScratchBufferForAcceStruct(const std::vector<AcceStructBuildData>& acceStructBuildData,
        uint32_t bufferAlignment, std::vector<VkDeviceAddress>& outScratchAddress) const;

    VkPhysicalDeviceAccelerationStructurePropertiesKHR mAcceStructProperties{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR};

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
};
} // namespace Bunny::Render
