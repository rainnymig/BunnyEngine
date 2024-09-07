#include "VulkanRendererFactory.h"

#include "VulkanRenderer.h"

namespace Bunny::Render
{
std::unique_ptr<Renderer> VulkanRendererFactory::makeRenderer(GLFWwindow* window)
{
    return std::make_unique<VulkanRenderer>(window);
}
} // namespace Bunny::Render