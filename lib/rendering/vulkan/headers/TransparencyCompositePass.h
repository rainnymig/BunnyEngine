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

    BunnyResult initPipelineLayout();
    BunnyResult initDescriptorLayouts();

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
    DescriptorAllocator mDescriptorAllocator;
    VkDescriptorSetLayout mImagesDescLayout;
    VkSampler mImageSampler;

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;

    AllocatedBuffer mVertexBuffer;
    AllocatedBuffer mIndexBuffer;
    //  vertices
    //  TL, BL, BR, TR
    std::array<ScreenQuadVertex, 4> mVertexData{
        ScreenQuadVertex{{-1, 1, 1},  {0, 1}},
        ScreenQuadVertex{{-1, -1, 1}, {0, 0}},
        ScreenQuadVertex{{1, -1, 1},  {1, 0}},
        ScreenQuadVertex{{1, 1, 1},   {1, 1}}
    };
    std::array<uint32_t, 6> mIndexData{0, 3, 1, 1, 3, 2};
};
} // namespace Bunny::Render
