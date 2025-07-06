#pragma once

#include <vulkan/vulkan.h>

#include <vector>

namespace Bunny::Render
{
//  acceleration structure geometry data
//  normally 1 AcceStructGeometryData corresponds to 1 mesh
//  each element in mGeometries and mBuildRanges corresponds to 1 surface in the mesh
struct AcceStructGeometryData
{
    std::vector<VkAccelerationStructureGeometryKHR> mGeometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> mBuildRanges;
    VkBuildAccelerationStructureFlagsKHR mBuildFlags{0};
};
} // namespace Bunny::Render
