#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"
#include "PbrGraphicsPass.h"
#include "ShaderData.h"

#include <glm/vec2.hpp>

#include <array>
#include <string_view>

namespace Bunny::Render
{
class WaveSpectrumTransformPass : public PbrGraphicsPass
{
  public:
    struct TimedSpectrumParams
    {
        float mTime;
        float mWidth;
        uint32_t mN; //  size of the sequence
    };

    struct FFTParams
    {
        uint32_t mN;         //  size of the sequence
        uint32_t mIsInverse; //  0: forward fft, 1: inversed fft
        uint32_t mDirection; //  0: fft on row, 1: fft on column
        float mTime;
    };

    WaveSpectrumTransformPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        uint32_t size, float width, std::string_view spectrumShaderPath = "wave_time_spectrum_comp.spv",
        std::string_view fftShaderPath = "wave_spectrum_transform_comp.spv");

    void draw() const override;

    void updateWaveTime(float time);
    void updateWidth(uint32_t size, float width);
    void updateSpectrumImage(const AllocatedImage* spectrumImage);

    //  transition the timed spectrum and fft transformed image for viewing
    void prepareCurrentFrameImagesForView();
    const AllocatedImage& getTimedSpectrumImage() const;
    const AllocatedImage& getIntermediateImage() const;
    const AllocatedImage& getHeightImage() const;

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;
    struct FrameData
    {
        //  desc set for timed spectrum
        VkDescriptorSet mTimedSpectrumDescSet;

        //  two descriptor sets, one for vertical fft and one for horizontal fft
        VkDescriptorSet mFirstFftImageDescSet;
        VkDescriptorSet mSecondFftImageDescSet;

        const AllocatedImage* mSpectrumImage;
        AllocatedImage mTimedSpectrumImage;
        AllocatedImage mIntermediateFftImage;
        AllocatedImage mWaveHeightImage;
    };

    BunnyResult initDescriptorLayouts();

    VkPipelineLayout mSpectrumPipelineLayout;
    VkPipeline mSpectrumPipeline;

    std::string_view mTimedSpectrumShaderPath;
    std::string_view mFftShaderPath;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;

    VkDescriptorSetLayout mImageDescLayout;
    DescriptorAllocator mDescriptorAllocator;

    TimedSpectrumParams mTimedSpectrumParams;
    mutable FFTParams mFFTParams{.mIsInverse = 1};
};
} // namespace Bunny::Render
