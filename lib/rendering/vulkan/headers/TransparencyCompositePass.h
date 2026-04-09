#pragma once

#include "PbrGraphicsPass.h"
#include "Descriptor.h"

#include <array>

namespace Bunny::Render
{
class TransparencyCompositePass : public PbrGraphicsPass
{
  public:
    TransparencyCompositePass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
        std::string_view vertShader = "screen_quad_vert.spv",
        std::string_view fragShader = "transparent_comp_frag.spv");

    void draw() const override;

    void setSceneRenderTarget(const AllocatedImage* sceneRenderTarget);
    void linkTransparentImages(const std::array<const AllocatedImage*, MAX_FRAMES_IN_FLIGHT>& accumImages,
        const std::array<const AllocatedImage*, MAX_FRAMES_IN_FLIGHT>& revealImages);

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        VkDescriptorSet mTransparentImageDescSet;

        const AllocatedImage* mAccumulateImage;
        const AllocatedImage* mRevealageImage;

        const AllocatedImage* mSceneRenderTarget;
    };

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
    DescriptorAllocator mDescriptorAllocator;

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;
};
} // namespace Bunny::Render
