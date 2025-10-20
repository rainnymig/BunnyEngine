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

    bool shouldUpdatePreviewTexture() const;
    void updateTextureForPreview();

    BunnyResult initDescriptorLayouts();

    TextureBank* mTextureBank;

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
    PreviewParams mPreviewParams;

    VkDescriptorSetLayout mTextureDescSetLayout;
    DescriptorAllocator mDescriptorAllocator;

    IdType mTex2dIdPreviewing = 0; //  the texture 2D which the current descriptor is bound to
    IdType mTex3dIdPreviewing = 0; //  the texture 3D which the current descriptor is bound to
    IdType mTex2dIdToPreview = 0;  //  the texture 2D to be previewed
    IdType mTex3dIdToPreview = 0;  //  the texture 3D to be previewed

    bool mIsActive = false;            //  is texture preview pass active at all
    bool mTexturePreviewReady = false; //  do we have a valid texture to preview
};
} // namespace Bunny::Render
