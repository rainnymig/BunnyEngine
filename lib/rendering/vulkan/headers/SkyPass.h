#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"

#include <glm/matrix.hpp>
#include <glm/vec4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

#include <array>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;

class SkyPass
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

    SkyPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    void draw() const;

  private:
    struct FrameData
    {
        VkDescriptorSet mCloudDescSet;   //  contains cloud rendering data
        VkDescriptorSet mTextureDescSet; //  contains texture images for render and render results

        AllocatedImage mRenderedCloudTexture;
        AllocatedImage mFogShadowTexture;
        AllocatedImage mDepthTexture;
        AllocatedImage mLastCloudTexture;
    };

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;

    CloudData mCloudData;
    CloudRenderParams mCloudRenderParams;

    AllocatedBuffer mCloudDataBuffer;
    AllocatedBuffer mCloudRenderParamsBuffer;
    AllocatedImage mMainNoiseTexture;
    AllocatedImage mDetailNoiseTexture;
    AllocatedImage mBlueNoiseTexture;
    AllocatedImage mWeatherNoiseTexture;
};
} // namespace Bunny::Render
