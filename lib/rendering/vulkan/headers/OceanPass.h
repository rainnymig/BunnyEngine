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

class OceanPass : PbrGraphicsPass
{
  public:
    struct SineWaveOctave
    {
        glm::vec2 mK;
        float mA;
        float mPhi;
    };

    static constexpr uint32_t OCTAVE_COUNT = 4;
    static constexpr uint32_t GRID_SIZE = 1024;
    static constexpr uint32_t MESH_THREAD_COUNT_X = 8;
    static constexpr uint32_t MESH_THREAD_COUNT_Y = 8;

    struct WaveFieldParams
    {
        SineWaveOctave mOctaves[OCTAVE_COUNT];
        glm::vec2 mGridOrigin;
        float mGridWidth;
    };

    struct WorldParams
    {
        glm::mat4 mMvpMatrix;
        float mElapsedTime;
        float mDeltaTime;
    };

    OceanPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        std::string_view meshShaderPath = "wave_mesh.spv", std::string_view fragShaderPath = "wave_frag.spv");

    void draw() const override;

    void updateWorldParams(const glm::mat4& mvpMatrix, float elapsedTime, float deltaTime);
    void linkLightAndCameraData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData);
    void updateRenderTarget(const AllocatedImage* renderTarget);

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        VkDescriptorSet mMeshDescSet;
        VkDescriptorSet mFragDescSet;

        const AllocatedImage* mRenderTarget;
    };

    BunnyResult initDescriptorLayouts();

    std::string_view mMeshShaderPath;
    std::string_view mFragShaderPath;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
    VkDescriptorSetLayout mMeshDescLayout;
    VkDescriptorSetLayout mFragDescLayout;
    DescriptorAllocator mDescriptorAllocator;

    WaveFieldParams mWaveParams;
    WorldParams mWorldParams;
    AllocatedBuffer mWaveParamsBuffer;
    AllocatedBuffer mWorldParamsBuffer;
};
} // namespace Bunny::Render
