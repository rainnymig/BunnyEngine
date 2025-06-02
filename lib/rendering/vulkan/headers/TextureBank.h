#pragma once

#include "BunnyResult.h"
#include "Fundamentals.h"

#include <vulkan/vulkan.h>

#include <span>
#include <string>
#include <vector>
#include <unordered_map>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class DescriptorWriter;

class TextureBank
{
  public:
    TextureBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    BunnyResult initialize();
    BunnyResult addTexture(const char* filePath, VkFormat format, IdType& outId);
    BunnyResult addDescriptorSetWrite(uint32_t binding, DescriptorWriter& outWriter) const;
    void cleanup();

  private:
    BunnyResult createSampler();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;

    std::vector<AllocatedImage> mTextures;
    std::unordered_map<std::string_view, IdType> mTexturePathToIds;
    VkSampler mImageSampler;
};
} // namespace Bunny::Render
