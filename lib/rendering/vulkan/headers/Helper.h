#pragma once

#include <vector>
#include <string_view>
#include <fstream>

#include <vulkan/vulkan.h>
#include <optional>

namespace Bunny::Render
{
VkCommandPoolCreateInfo makeCommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo makeCommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t bufferCount = 1);
VkCommandBufferSubmitInfo makeCommandBufferSubmitInfo(VkCommandBuffer commandBuffer);
VkSubmitInfo2 makeSubmitInfo2(VkCommandBufferSubmitInfo* cmdBufferSubmit, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo);
VkPipelineShaderStageCreateInfo makeShaderStageCreateInfo(
    VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entryPoint = "main");
VkRenderingAttachmentInfo makeColorAttachmentInfo(
    VkImageView view, VkClearValue* clearValue, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
VkRenderingAttachmentInfo makeDepthAttachmentInfo(
    VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
VkRenderingInfo makeRenderingInfo(
    VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment);

void transitionImageLayout(
    VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

} // namespace Bunny::Render