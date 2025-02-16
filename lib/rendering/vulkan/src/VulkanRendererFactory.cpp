#include "VulkanRendererFactory.h"

#include "VulkanRenderer.h"
#include "VulkanRendererNext.h"

namespace Bunny::Render
{
std::unique_ptr<Renderer> VulkanRendererFactory::makeRenderer(GLFWwindow* window)
{
    return std::make_unique<VulkanRendererNext>(window);
}
} // namespace Bunny::Render