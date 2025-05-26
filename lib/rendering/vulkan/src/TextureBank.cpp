#include "TextureBank.h"

namespace Bunny::Render
{
TextureBank::TextureBank(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
}
} // namespace Bunny::Render
