#pragma once

#include "BunnyResult.h"

#include <span>
#include <string>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;

class TextureBank
{
  public:
    TextureBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer);

    BunnyResult initialize();
    BunnyResult addTextures(std::span<std::string> filePaths);
    BunnyResult bindTextures();
    void cleanup();

  private:
    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
};
} // namespace Bunny::Render
