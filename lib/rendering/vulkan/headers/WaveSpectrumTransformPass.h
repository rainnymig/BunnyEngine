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
    struct FFTParams
    {
        uint32_t mN;         //  size of the sequence
        uint32_t mIsInverse; //  0: forward fft, 1: inversed fft
        uint32_t mDirection; //  0: fft on row, 1: fft on column
        float mTime;
    };

    WaveSpectrumTransformPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        std::string_view shaderPath = "wave_spectrum_transform_comp.spv");

    void draw() const override;

    void updateWaveTime(float time);
    void updateFFTSize(uint32_t size);
    void updateSpectrumImage(const AllocatedImage* spectrumImage);

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;
    struct FrameData
    {
        //  two descriptor sets, one for vertical fft and one for horizontal fft
        VkDescriptorSet mFirstFftImageDescSet;
        VkDescriptorSet mSecondFftImageDescSet;

        const AllocatedImage* mSpectrumImage;
        AllocatedImage mIntermediateFftImage;
        AllocatedImage mWaveHeightImage;
    };

    BunnyResult initDescriptorLayouts();

    std::string_view mShaderPath;

    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;

    VkDescriptorSetLayout mImageDescLayout;
    DescriptorAllocator mDescriptorAllocator;

    mutable FFTParams mFFTParams{.mIsInverse = 1};
};
} // namespace Bunny::Render
