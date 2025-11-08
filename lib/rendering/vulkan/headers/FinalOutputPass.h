#pragma once

#include "PbrGraphicsPass.h"
#include "Fundamentals.h"
#include "Descriptor.h"

#include <array>
#include <string_view>

namespace Bunny::Render
{
class FinalOutputPass : public PbrGraphicsPass
{
  public:
    FinalOutputPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        std::string_view vertShader = "screen_quad_vert.spv", std::string_view fragShader = "final_output_frag.spv");

    void draw() const override;

    void updateInputTextures(const AllocatedImage* cloudTexture, const AllocatedImage* fogShadowTexture,
        const AllocatedImage* renderedSceneTexture);

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;

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

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;

    VkDescriptorSetLayout mTextureDescSetLayout = nullptr;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
};
} // namespace Bunny::Render
