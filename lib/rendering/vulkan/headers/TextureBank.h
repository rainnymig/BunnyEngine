#pragma once

#include "BunnyResult.h"
#include "Fundamentals.h"

#include <vulkan/vulkan.h>

#include <span>
#include <string>
#include <vector>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;

class TextureBank
{
  public:
    TextureBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    BunnyResult initialize();
    BunnyResult addTexture(const char* filePath, VkFormat format, IdType& outId);
    BunnyResult updateDescriptorSet(VkDescriptorSet descriptorSet, uint32_t binding) const;
    void cleanup();

  private:
    BunnyResult createSampler();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;

    std::vector<AllocatedImage> mTextures;
    VkSampler mImageSampler;
};
} // namespace Bunny::Render
