#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"
#include "PbrGraphicsPass.h"

#include <glm/matrix.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <array>
#include <string_view>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class TextureBank;

class SkyPass : public PbrGraphicsPass
{
  public:
    struct CloudData
    {
        glm::vec4 mCloudNoiseDimension;
        glm::vec3 mDetailNoiseDimension;
        float mCloudCoverage;
        glm::vec3 mWeatherMapDimension;
        float mLightMarchStepSize;
        glm::uvec2 mRenderResolution;
        glm::vec2 mCloudRegionMinXZ;
        glm::vec2 mCloudRegionMaxXZ;
        float mG1; //  henyey greestein coefficient
        float mG2;
        float mExtinctionCoef;
        float mScatteringCoef;
        float mDensityModifier;
        float mMinRayMarchStepSize;
        float mZNear;
        float mZFar;
        float mDetailErodeModCoef;
        float mLodStartDistance;
        float mLodStageLength;
        float mCloudRegionR1; //  the distance from the bottom of the cloud region to the center of the Earth
        float mCloudRegionR2; //  the distance from the top of the cloud region to the center of the Earth
        float mCloudRegionCy; //  the distance from center of the game world (0, 0, 0) to the center of the Earth
    };

    struct CloudRenderParams
    {
        glm::mat4 mPrevViewProj; //  view projection matrix of previous frame

        glm::vec4 mCameraFrameWorldTL; //  top left
        glm::vec4 mCameraFrameWorldTR; //  top right
        glm::vec4 mCameraFrameWorldBL; //  bottom left
        glm::vec4 mCameraFrameWorldBR; //  bottom right

        glm::vec3 mCameraPosition;

        float mElapsedTime;
        uint32_t mDitherIdx;
    };

    SkyPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        TextureBank* textureBank, std::string_view cloudShaderPath = "sky_comp.spv");

    void draw() const override;

    void updateFrameData();

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        //  because we want to have last frame cloud texture
        //  we create two sets of descriptor sets
        //  so they can bind there render target and last frame cloud texture
        //  to different allocated images
        //  this is not the same as multi frames in flight
        //  because each frame in flight has two sets of desc sets
        static constexpr int frameSequenceCount = 2;

        struct DescSets
        {
            VkDescriptorSet mCloudDescSet;   //  contains cloud rendering data
            VkDescriptorSet mTextureDescSet; //  contains texture images for render and render results
        };
        std::array<DescSets, frameSequenceCount> mDescriptors;

        AllocatedImage mCloudTexture1;
        AllocatedImage mCloudTexture2;
        AllocatedImage mFogShadowTexture;
        const AllocatedImage* mDepthTexture = nullptr; //  depth image created in renderer
        int mCurrentFrameSeqId = 0;

        void advanceFrameInSequence() { mCurrentFrameSeqId = (mCurrentFrameSeqId + 1) % frameSequenceCount; }
        const VkDescriptorSet* getCurrentDescSets() const { return &mDescriptors[mCurrentFrameSeqId].mCloudDescSet; }
    };

    BunnyResult initDescriptorLayouts();

    TextureBank* mTextureBank;
    std::string_view mCloudShaderPath;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;

    CloudData mCloudData;
    CloudRenderParams mCloudRenderParams;
    AllocatedBuffer mCloudDataBuffer;
    AllocatedBuffer mCloudRenderParamsBuffer;

    AllocatedImage mMainNoiseTexture;
    AllocatedImage mDetailNoiseTexture;
    AllocatedImage mBlueNoiseTexture;
    AllocatedImage mWeatherNoiseTexture;

    VkDescriptorSetLayout mCloudDescSetLayout;
    VkDescriptorSetLayout mTextureDescSetLayout;
    DescriptorAllocator mDescriptorAllocator;

    static constexpr uint32_t MAIN_CLOUD_NOISE_RESOLUTION = 128;
    static constexpr uint32_t DETAIL_CLOUD_NOISE_RESOLUTION = 32;
    static constexpr uint32_t WEATHER_RESOLUTION = 1024;
    static constexpr uint32_t BLUE_NOISE_RESOLUTION = 256;
    static constexpr uint32_t TEXTURE_2D_COUNT = 2;
    static constexpr uint32_t TEXTURE_3D_COUNT = 2;
};

} // namespace Bunny::Render
