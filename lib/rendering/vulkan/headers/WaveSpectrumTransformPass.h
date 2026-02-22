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
        uint32_t iteration;  //  currenct iteration, should be between 0 and log2(N);
    };

    struct WaveParams
    {
        float mU; //  displacement factor, should be >= 0
    };

    WaveSpectrumTransformPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        uint32_t size, float width, std::string_view spectrumShaderPath = "wave_time_spectrum_comp.spv",
        std::string_view bitReverseShaderPath = "fft_bit_reverse_comp.spv",
        std::string_view fftPingPongShaderPath = "fft_ping_pong_comp.spv",
        std::string_view waveConstructShaderPath = "wave_construct_comp.spv");

    void draw() const override;

    void updateWaveTime(float time);
    void updateWidth(uint32_t size, float width);
    void updateSpectrumImage(const AllocatedImage* spectrumImage);

    //  transition the timed spectrum and fft transformed image for viewing
    void prepareCurrentFrameImagesForView();
    const AllocatedImage& getHeightImage() const;
    const AllocatedImage& getWaveDisplacementImage() const;
    const AllocatedImage& getWaveNormalImage() const;

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;
    struct FrameData
    {
        const AllocatedImage* mSpectrumImage;

        AllocatedImage mTimedSpectrumPing;
        AllocatedImage mTimedSpectrumPong;
        AllocatedImage mSlopeSpectrumPing;
        AllocatedImage mSlopeSpectrumPong;
        AllocatedImage mDisplaceSpectrumPing;
        AllocatedImage mDisplaceSpectrumPong;
        AllocatedImage mDisplaceSlopeSpectrumPing;
        AllocatedImage mDisplaceSlopeSpectrumPong;

        AllocatedImage mWaveDisplacementImage;
        AllocatedImage mWaveNormalImage;
        //  self intersection image?

        bool mIsOutputToImagePong; //  now all images have same dimension so can share the same mIsOutputToImagePong
        mutable DescriptorAllocator mDescriptorAllocator;
    };

    BunnyResult initDescriptorLayouts();

    void computeTimedSpectrum() const;

    //  perform 2D fast fourier transform on input image
    //  isInverse: indicates whether to perform fft or inverse fft
    //  isOutputToBuffer: the fft/ifft is performed using two images in a ping-pong way,
    //                    so the result can be in the inputImage or in the bufferImage
    //                    depending on the size of the image
    //                    (which determines the number of iterations).
    //                    Therefore, to retrieve the result,
    //                    check isOutputToBuffer afterwards to see where the result is.
    //                    if true, output is bufferImage, otherwise output is inputImage
    void fastFourierTransform(const AllocatedImage& inputImage, const AllocatedImage& bufferImage, uint32_t N,
        bool isInverse, bool& isOutputToBuffer) const;
    void fastFourierTransformOneDir(const AllocatedImage& inputImage, const AllocatedImage& bufferImage, uint32_t N,
        uint32_t direction, bool isInverse, bool& isOutputToBuffer) const;

    void constructWave() const;

    void showImguiControlPanel();

    VkPipelineLayout mSpectrumPipelineLayout;
    VkPipeline mSpectrumPipeline;
    VkPipelineLayout mBitReversePipelineLayout;
    VkPipeline mBitReversePipeline;
    VkPipelineLayout mWaveConstructPipelineLayout;
    VkPipeline mWaveConstructPipeline;

    std::string_view mTimedSpectrumShaderPath;
    std::string_view mBitReverseShaderPath;
    std::string_view mFftShaderPath;
    std::string_view mWaveConstructShaderPath;

    mutable std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;

    VkDescriptorSetLayout mTimedSpectrumDescLayout;
    VkDescriptorSetLayout mImageDescLayout;
    VkDescriptorSetLayout mWaveConstructDescLayout;

    TimedSpectrumParams mTimedSpectrumParams;
    mutable FFTParams mFFTParams{.mIsInverse = 1};
    WaveParams mWaveParams{.mU = 1.15f};

    bool mFreezeWaveTime = false;
};
} // namespace Bunny::Render
