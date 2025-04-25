#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"

#include "BunnyResult.h"
#include "BunnyGuard.h"

#include <vulkan/vulkan.h>
#include <string_view>
#include <memory>
#include <array>
#include <string>

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
    const MaterialPipeline& getMaterialPipeline() const { return mPipeline; }
    virtual void cleanup() = 0;
    virtual ~Material();

    IdType mId;

  protected:
    MaterialPipeline mPipeline;
};

class BasicBlinnPhongMaterial : public Material
{
  public:
    class Builder
    {
      public:
        void setColorAttachmentFormat(VkFormat format) { mColorFormat = format; }
        void setDepthFormat(VkFormat format) { mDepthFormat = format; }
        void setShaderPaths(const std::string& vertexShader, const std::string& fragmentShader)
        {
            mVertexShaderPath = vertexShader;
            mFragmentShaderPath = fragmentShader;
        }
        std::unique_ptr<BasicBlinnPhongMaterial> buildMaterial(VkDevice device) const;

      private:
        BunnyResult buildPipeline(VkDevice device, VkDescriptorSetLayout sceneLayout,
            VkDescriptorSetLayout objectLayout, MaterialPipeline& outPipeline) const;
        BunnyResult buildDescriptorSetLayouts(
            VkDevice device, VkDescriptorSetLayout& outSceneLayout, VkDescriptorSetLayout& outObjectLayout) const;

        std::string mVertexShaderPath{"./basic_instanced_vert.spv"};
        std::string mFragmentShaderPath{"./basic_updated_frag.spv"};
        VkFormat mColorFormat = VK_FORMAT_B8G8R8A8_SRGB;
        VkFormat mDepthFormat = VK_FORMAT_D32_SFLOAT;
    };

    BasicBlinnPhongMaterial(Base::BunnyGuard<Builder> guard, VkDevice device);
    virtual void cleanup() override;
    void cleanupPipeline();

    constexpr std::string_view getName() const { return "Basic Blinn Phong"; }
    MaterialInstance makeInstance();

    VkDescriptorSetLayout getSceneDescSetLayout() const { return mSceneDescSetLayout; }
    VkDescriptorSetLayout getObjectDescSetLayout() const { return mObjectDescSetLayout; }

  private:
    VkDevice mDevice;
    VkDescriptorSetLayout mSceneDescSetLayout = nullptr;
    VkDescriptorSetLayout mObjectDescSetLayout = nullptr;

    // DescriptorAllocator mDescriptorAllocator;
    // VkDescriptorSetLayout mDescriptorSetLayout;
};
} // namespace Bunny::Render
