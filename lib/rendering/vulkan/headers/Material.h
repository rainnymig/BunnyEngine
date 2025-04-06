#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"

#include "BunnyGuard.h"

#include <vulkan/vulkan.h>
#include <string_view>
#include <memory>
#include <array>

namespace Bunny::Render
{
struct MaterialPipeline
{
    VkPipeline mPipeline;
    VkPipelineLayout mPipelineLayout;
};

struct MaterialInstance
{
    IdType mId;
    MaterialPipeline* mpBaseMaterial;
    VkDescriptorSet mDescriptorSet = nullptr;
};

class Material
{
  public:
    IdType getId() const { return mId; }
    const MaterialPipeline& getMaterialPipeline() const { return mPipeline; }
    virtual void cleanup() = 0;
    virtual ~Material() {}

  protected:
    IdType mId;
    MaterialPipeline mPipeline;
};

class BasicBlinnPhongMaterial : public Material
{
  public:
    class Builder
    {
      public:
        void setPushConstantRanges(std::span<VkPushConstantRange> ranges)
        {
            mPushConstantRanges.assign(ranges.begin(), ranges.end());
        }
        void setColorAttachmentFormat(VkFormat format) { mColorFormat = format; }
        void setDepthFormat(VkFormat format) { mDepthFormat = format; }
        void setSceneDescriptorSetLayout(VkDescriptorSetLayout layout) { mSceneDescSetLayout = layout; }
        void setObjectDescriptorSetLayout(VkDescriptorSetLayout layout) { mObjectDescSetLayout = layout; }
        std::unique_ptr<BasicBlinnPhongMaterial> buildMaterial(VkDevice device) const;

      private:
        MaterialPipeline buildPipeline(VkDevice device) const;

        std::vector<VkPushConstantRange> mPushConstantRanges;
        VkFormat mColorFormat = VK_FORMAT_B8G8R8A8_SRGB;
        VkFormat mDepthFormat = VK_FORMAT_D32_SFLOAT;
        VkDescriptorSetLayout mSceneDescSetLayout = nullptr;
        VkDescriptorSetLayout mObjectDescSetLayout = nullptr;
    };

    BasicBlinnPhongMaterial(Base::BunnyGuard<Builder> guard, VkDevice device);
    virtual void cleanup() override;
    void cleanupPipeline();

    constexpr std::string_view getName() const { return "Basic Blinn Phong"; }
    MaterialInstance makeInstance();

  private:
    static constexpr std::string_view VERTEX_SHADER_PATH{"./basic_instanced_vert.spv"};
    static constexpr std::string_view FRAGMENT_SHADER_PATH{"./basic_updated_frag.spv"};

    VkDevice mDevice;

    // void buildDescriptorSetLayout(VkDevice device);

    // DescriptorAllocator mDescriptorAllocator;
    // VkDescriptorSetLayout mDescriptorSetLayout;
};
} // namespace Bunny::Render
