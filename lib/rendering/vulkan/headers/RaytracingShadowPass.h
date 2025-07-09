#pragma once

#include "PbrGraphicsPass.h"

namespace Bunny::Render
{
class RaytracingShadowPass : public PbrGraphicsPass
{
  public:
    RaytracingShadowPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank);

    virtual void draw() const override;

  protected:
    virtual BunnyResult initPipeline() override;

  private:
    using super = PbrGraphicsPass;

    BunnyResult buildPipelineLayout();
    BunnyResult buildRaytracingDescSetLayouts();
    BunnyResult buildShaderBindingTable();
    void queryRaytracingProperties();

    AllocatedBuffer mShaderBindingTableBuffer;
    VkStridedDeviceAddressRegionKHR mRayGenRegion;
    VkStridedDeviceAddressRegionKHR mMissRegion;
    VkStridedDeviceAddressRegionKHR mHitRegion;

    VkPhysicalDeviceRayTracingPipelinePropertiesKHR mRaytracingProperties{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
};

} // namespace Bunny::Render
