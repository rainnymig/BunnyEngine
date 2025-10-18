#include "SkyPass.h"

namespace Bunny::Render
{

SkyPass::SkyPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer)
{
}

void SkyPass::draw() const
{
}

} // namespace Bunny::Render
