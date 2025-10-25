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
        std::string_view vertShader = "screen_quad_vert.spv", std::string_view fragShader = "texture_preview_frag.spv");

    void draw() const override;
    void showImguiControls();
    void setIsActive(bool isActive);
    void updateTextureForPreview();

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
        uint32_t mIs3d = 0;
    };

    bool shouldUpdatePreviewTexture() const;
    void updateScreenQuad(float aspectRatio); //  aspectRatio = width/height

    BunnyResult initDescriptorLayouts();

    TextureBank* mTextureBank;

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;

    AllocatedBuffer mVertexBuffer;
    AllocatedBuffer mIndexBuffer;
    //  vertices
    //  TL, BL, BR, TR
    std::array<ScreenQuadVertex, 4> mVertexData{
        ScreenQuadVertex{{-1, 1, 0.5},  {0, 1}},
        ScreenQuadVertex{{-1, -1, 0.5}, {0, 0}},
        ScreenQuadVertex{{1, -1, 0.5},  {1, 0}},
        ScreenQuadVertex{{1, 1, 0.5},   {1, 1}}
    };
    std::array<uint32_t, 6> mIndexData{0, 3, 1, 1, 3, 2};

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
    PreviewParams mPreviewParams;

    VkDescriptorSetLayout mTextureDescSetLayout;
    DescriptorAllocator mDescriptorAllocator;

    int mTex2dIdPreviewing = -1; //  the texture 2D which the current descriptor is bound to
    int mTex3dIdPreviewing = -1; //  the texture 3D which the current descriptor is bound to
    int mTex2dIdToPreview = 0;   //  the texture 2D to be previewed
    int mTex3dIdToPreview = 0;   //  the texture 3D to be previewed

    float mCurrentTexAspectRatio = 0;

    bool mIsActive = false;            //  is texture preview pass active at all
    bool mTexturePreviewReady = false; //  do we have a valid texture to preview
};
} // namespace Bunny::Render
