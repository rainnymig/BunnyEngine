#pragma once

#include "BunnyResult.h"
#include "Fundamentals.h"

#include <volk.h>

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
    BunnyResult addTextureFromMemory(unsigned char* data, int dataLength, VkFormat, IdType& outId);
    BunnyResult addTexture3d(
        const char* filePath, VkFormat format, uint32_t width, uint32_t height, uint32_t depth, IdType& outId);
    BunnyResult addDescriptorSetWrite(uint32_t binding, DescriptorWriter& outWriter) const;
    bool getTexture(IdType id, AllocatedImage& outTexture) const; //  return true when success, otherwise false
    const std::vector<AllocatedImage>& getAllTextures() const;
    bool getTexture3d(IdType id, AllocatedImage& outTexture) const; //  return true when success, otherwise false
    const std::vector<AllocatedImage>& getAllTextures3d() const;
    VkSampler getSampler() const;

    void cleanup();

  private:
    BunnyResult createSampler();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;

    std::vector<AllocatedImage> mTextures;
    std::vector<AllocatedImage> mTextures3d;
    std::unordered_map<std::string_view, IdType> mTexturePathToIds;
    VkSampler mImageSampler;
};
} // namespace Bunny::Render
