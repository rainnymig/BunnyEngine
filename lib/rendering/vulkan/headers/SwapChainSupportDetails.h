#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Bunny::Render
{
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
} // namespace Bunny::Render