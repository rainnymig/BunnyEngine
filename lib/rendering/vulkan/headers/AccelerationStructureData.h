#pragma once

#include "Fundamentals.h"

#include <volk.h>

#include <vector>

namespace Bunny::Render
{
//  geometry data used as input to build acceleration structure
//  normally 1 AcceStructGeometryData corresponds to 1 mesh
//  each element in mGeometries and mBuildRanges corresponds to 1 surface in the mesh
struct AcceStructGeometryData
{
    std::vector<VkAccelerationStructureGeometryKHR> mGeometries;
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> mBuildRanges;
    VkBuildAccelerationStructureFlagsKHR mBuildFlags{0};
};

struct BuiltAccelerationStructure
{
    VkAccelerationStructureKHR mAcceStruct = VK_NULL_HANDLE;
    AllocatedBuffer mBuffer;
    VkDeviceAddress mAcceAddress;
};

} // namespace Bunny::Render
