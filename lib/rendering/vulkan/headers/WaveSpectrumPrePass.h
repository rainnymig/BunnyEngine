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
class WaveSpectrumPrePass : public PbrGraphicsPass
{
  public:
    struct WaveSpectrumData
    {
        glm::vec2 wind; //  wind vector
        float width;    //  the horizontal dimension of the whole wave grid area, in meters
        int N;          //  size of the spectrum (in one dimension)
        float A;        //  Phillips spectrum magnitude coefficient
    };

    static constexpr float AREA_WIDTH = 102.4f;
    static constexpr int GRID_N = 256;

    WaveSpectrumPrePass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        std::string_view shaderPath = "wave_spectrum_comp.spv");

    void draw() const override;

    const AllocatedImage& getSpectrumImage() const { return mSpectrumImage; }
    const float getWidth() const { return AREA_WIDTH; }
    const int getGridN() const { return GRID_N; }

    //  transition the spectrum image for viewing
    void prepareSpectrumImageForView();

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;

    struct FrameData
    {
        VkDescriptorSet mImageDescSet;
        VkDescriptorSet mSpectrumDescSet;
    };

    BunnyResult initDescriptorLayouts();
    BunnyResult createRandomValueImage();

    std::string_view mShaderPath;

    FrameData mFrame;

    AllocatedImage mSpectrumImage;
    WaveSpectrumData mWaveSpectrumData;
    AllocatedBuffer mWaveSpectrumBuffer;
    AllocatedImage mStdNormalDistImage;

    DescriptorAllocator mDescriptorAllocator;
    VkDescriptorSetLayout mImageDescLayout;
    VkDescriptorSetLayout mSpectrumDescLayout;
};
} // namespace Bunny::Render
