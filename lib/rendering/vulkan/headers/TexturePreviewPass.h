#pragma once

#include "Fundamentals.h"
#include "PbrGraphicsPass.h"
#include "Descriptor.h"

#include <string_view>
#include <array>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class TextureBank;

class TexturePreviewPass : public PbrGraphicsPass
{
  public:
    TexturePreviewPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank, TextureBank* textureBank,
        std::string_view vertShader = "screen_quad_vert.spv", std::string_view fragShader = "texture_display_frag.spv");

    void draw() const override;
    void showImguiControls();

    void setIsActive(bool isActive);

  protected:
    virtual BunnyResult initPipeline();
    virtual BunnyResult initDescriptors();
    virtual BunnyResult initDataAndResources();

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        VkDescriptorSet mTextureDescSet;
    };

    struct PreviewParams
    {
        float mUvZ = 0; //  the z value when previewing a 3D texture
        bool mIs3d = false;
    };

    TextureBank* mTextureBank;

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
    PreviewParams mPreviewParams;

    VkDescriptorSetLayout mTextureDescSetLayout;

    uint32_t mCurrentPreviewImageIdx = 0;

    bool mIsActive = false;
};
} // namespace Bunny::Render
