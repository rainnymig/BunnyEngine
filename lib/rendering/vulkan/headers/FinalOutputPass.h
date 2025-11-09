#pragma once

#include "PbrGraphicsPass.h"
#include "Fundamentals.h"
#include "Descriptor.h"
#include "Vertex.h"

#include <array>
#include <string_view>

namespace Bunny::Render
{
class TextureBank;

class FinalOutputPass : public PbrGraphicsPass
{
  public:
    FinalOutputPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const TextureBank* textureBank, std::string_view vertShader = "screen_quad_vert.spv",
        std::string_view fragShader = "final_output_frag.spv");

    void draw() const override;

    void updateInputTextures(const AllocatedImage* cloudTexture, const AllocatedImage* fogShadowTexture,
        const AllocatedImage* renderedSceneTexture);

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        VkDescriptorSet mTextureDescSet;
        DescriptorAllocator mDescriptorAllocator;

        const AllocatedImage* mCloudTexture = nullptr;
        const AllocatedImage* mFogShadowTexture = nullptr;
        const AllocatedImage* mRenderedSceneTexture = nullptr;
    };

    BunnyResult initDescriptorLayouts();

    const TextureBank* mTextureBank;

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

    VkDescriptorSetLayout mTextureDescSetLayout = nullptr;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
};
} // namespace Bunny::Render
