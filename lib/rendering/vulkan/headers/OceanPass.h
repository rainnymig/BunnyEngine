#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"
#include "PbrGraphicsPass.h"
#include "ShaderData.h"

#include <glm/matrix.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <string_view>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class TextureBank;
class Camera;

class OceanPass : public PbrGraphicsPass
{
  public:
    static constexpr uint32_t MESH_THREAD_COUNT_X = 8;
    static constexpr uint32_t MESH_THREAD_COUNT_Y = 8;

    struct WaveFieldParams
    {
        glm::vec2 mGridOrigin;
        float mGridCellWidth;
        float mGridAreaWidth;
    };

    struct WorldParams
    {
        glm::mat4 mMvpMatrix;
        float mElapsedTime;
        float mDeltaTime;
    };

    struct WavePushParams
    {
        uint32_t mMaterialIdx;
    };

    OceanPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const TextureBank* textureBank, PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
        float waveAreaWidth, uint32_t patternAreaGridCount, uint32_t totalGridCount,
        std::string_view meshShaderPath = "wave_fft_mesh.spv", std::string_view fragShaderPath = "wave_frag.spv");

    void draw() const override;

    void prepareFrameDescriptors();

    void updateWorldParams(const glm::mat4& mvpMatrix, float elapsedTime, float deltaTime);
    void linkLightAndCameraData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData);
    void updateRenderTarget(const AllocatedImage* renderTarget);
    void updateWaveTextures(const AllocatedImage* vertexDisplacementTex, const AllocatedImage* vertexNormalTex);

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        VkDescriptorSet mMeshDescSet;
        VkDescriptorSet mWaveImageDescSet;
        VkDescriptorSet mFragDescSet;
        VkDescriptorSet mMaterialDescSet;

        const AllocatedImage* mRenderTarget;
        const AllocatedImage* mVertexDisplacementImage;
        const AllocatedImage* mVertexNormalImage;

        DescriptorAllocator mDescriptorAllocator;
    };

    BunnyResult initDescriptorLayouts();

    const TextureBank* mTextureBank;

    std::string_view mMeshShaderPath;
    std::string_view mFragShaderPath;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
    VkDescriptorSetLayout mMeshDescLayout;
    VkDescriptorSetLayout mWaveImageDescLayout;
    VkDescriptorSetLayout mFragDescLayout;

    WaveFieldParams mWaveParams;
    WorldParams mWorldParams;
    WavePushParams mWavePushParams;
    uint32_t mTotalWaveGridCount;
    AllocatedBuffer mWaveParamsBuffer;
    AllocatedBuffer mWorldParamsBuffer;
    const AllocatedBuffer* mLightDataBuffer;
    const AllocatedBuffer* mCameraDataBuffer;
};
} // namespace Bunny::Render
