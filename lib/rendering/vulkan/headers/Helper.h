#pragma once

#include "Vertex.h"
#include "MeshBank.h"
#include "MaterialBank.h"
#include "Fundamentals.h"

#include <volk.h>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>

#include <vector>
#include <string_view>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <memory>
#include <span>

namespace Bunny::Render
{
VkCommandPoolCreateInfo makeCommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
VkCommandBufferAllocateInfo makeCommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t bufferCount = 1);
VkCommandBufferSubmitInfo makeCommandBufferSubmitInfo(VkCommandBuffer commandBuffer);
VkSubmitInfo2 makeSubmitInfo2(VkCommandBufferSubmitInfo* cmdBufferSubmit, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo);
VkPipelineShaderStageCreateInfo makeShaderStageCreateInfo(
    VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entryPoint = "main");
VkRayTracingShaderGroupCreateInfoKHR makeRayTracingShaderGroupCreateInfoKHR();
VkRenderingAttachmentInfo makeColorAttachmentInfo(
    VkImageView view, VkClearValue* clearValue, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
VkRenderingAttachmentInfo makeDepthAttachmentInfo(
    VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
VkRenderingInfo makeRenderingInfo(VkExtent2D renderExtent, uint32_t colorAttachmentCount,
    VkRenderingAttachmentInfo* colorAttachments, VkRenderingAttachmentInfo* depthAttachment);
VkBufferMemoryBarrier makeBufferMemoryBarrier(VkBuffer buffer, uint32_t queueIndex);
VkImageMemoryBarrier makeImageMemoryBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);

//  find the largest power of 2 that's less than val
uint32_t findPreviousPow2(uint32_t val);

bool hasFlag(VkFlags testFlag, VkFlags targetFlag);

VkTransformMatrixKHR convertToVkTransformMatrix(const glm::mat4& glmMat);

template <typename T>
void copyDataToBuffer(const T& data, AllocatedBuffer& buffer)
{
    void* mappedData = buffer.mAllocationInfo.pMappedData;
    memcpy(mappedData, &data, sizeof(T));
}

template <typename T>
void copyDataArrayToBuffer(const std::span<T> data, AllocatedBuffer& buffer)
{
    void* mappedData = buffer.mAllocationInfo.pMappedData;
    memcpy(mappedData, data.data(), data.size_bytes());
}

} // namespace Bunny::Render