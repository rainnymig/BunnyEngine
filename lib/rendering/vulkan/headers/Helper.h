#pragma once

#include <vector>
#include <string_view>
#include <fstream>

#include <vulkan/vulkan.h>

namespace Bunny::Render
{
std::vector<std::byte> readShaderFile(std::string_view path);

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
} // namespace Bunny::Render