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
        float width;    //  the horizontal dimension of the whole wave grid area, in meters
        float ksiR;     //  a random value from a standard normal distribution (mean 0, standard deviation 1)
        float ksiI;     //  another random value from a standard normal distribution (mean 0, standard deviation 1)
        int N;          //  size of the spectrum (in one dimension)
        glm::vec2 wind; //  wind vector
        float A;        //  Phillips spectrum magnitude coefficient
    };

    static constexpr float AREA_WIDTH = 51.2f;
    static constexpr int GRID_N = 512;

    WaveSpectrumPrePass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        std::string_view shaderPath = "wave_mesh.spv");

    void draw() const override;

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

    std::string_view mShaderPath;

    FrameData mFrame;

    AllocatedImage mSpectrumImage;
    WaveSpectrumData mWaveSpectrumData;
    AllocatedBuffer mWaveSpectrumBuffer;

    DescriptorAllocator mDescriptorAllocator;
    VkDescriptorSetLayout mImageDescLayout;
    VkDescriptorSetLayout mSpectrumDescLayout;
};
} // namespace Bunny::Render
