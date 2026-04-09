#pragma once

#include "PbrGraphicsPass.h"
#include "Descriptor.h"

namespace Bunny::Render
{
class TransparencyAccumulatePass : public PbrGraphicsPass
{
  public:
    TransparencyAccumulatePass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
        std::string_view vertShader = "pbr_culled_instanced_vert.spv",
        std::string_view fragShader = "transparent_accum_frag.spv");

    void draw() const override;

    void linkWorldData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData);
    void linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);
    void linkShadowData(std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> shadowImageViews);
    void setDrawCommandsBuffer(const AllocatedBuffer& buffer);

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        VkDescriptorSet mWorldDescSet;
        VkDescriptorSet mObjectDescSet;
        VkDescriptorSet mMaterialDescSet;
        VkDescriptorSet mEffectDescSet; // for shadows

        AllocatedImage mAccumulateImageMultiSampled;
        AllocatedImage mRevealageImageMultiSampled;
        AllocatedImage mAccumulateImage;
        AllocatedImage mRevealageImage;
    };

    const AllocatedBuffer* mDrawCommandsBuffer;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
    DescriptorAllocator mDescriptorAllocator;

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;
};
} // namespace Bunny::Render
