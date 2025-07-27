#pragma once

#include "PbrGraphicsPass.h"
#include "ShaderData.h"
#include "Descriptor.h"
#include "Fundamentals.h"

#include <vulkan/vulkan.h>

#include <array>

namespace Bunny::Render
{
class RaytracingShadowPass : public PbrGraphicsPass
{
  public:
    RaytracingShadowPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank);

    virtual void draw() const override;

    void updateVertIdxBufferData(VkDeviceAddress vertBufAddress, VkDeviceAddress idxBufAddress);
    void linkWorldData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData);
    void linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);
    void linkTopLevelAccelerationStructure(VkAccelerationStructureKHR acceStruct);

    [[nodiscard]] std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> getOutImageViews() const;

  protected:
    virtual BunnyResult initPipeline() override;
    virtual BunnyResult initDescriptors() override;
    virtual BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        VkDescriptorSet mWorldDescSet;
        VkDescriptorSet mObjectDescSet;
        VkDescriptorSet mMaterialDescSet;
        VkDescriptorSet mRtDataDescSet;

        AllocatedImage mOutImage;
    };

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

    VertexIndexBufferData mVertIdxBufData;
    AllocatedBuffer mVertIdxBufBuffer;

    VkDescriptorSetLayout mObjectDescSetLayout;
    VkDescriptorSetLayout mRtDataDescSetLayout;

    DescriptorAllocator mDescriptorAllocator;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
};

} // namespace Bunny::Render
