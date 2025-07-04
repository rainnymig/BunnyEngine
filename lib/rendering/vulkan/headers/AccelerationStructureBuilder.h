#pragma once

#include "AccelerationStructureData.h"

#include <vector>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;

class AccelerationStructureBuilder
{
  public:
    void buildBottomLevelAccelerationStructures(
        const std::vector<BlasGeometryData>& blasData, VkBuildAccelerationStructureFlagsKHR flags);

  private:
    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
};
} // namespace Bunny::Render
