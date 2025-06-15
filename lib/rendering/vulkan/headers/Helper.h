#pragma once

#include "Vertex.h"
#include "MeshBank.h"
#include "MaterialBank.h"

#include <vulkan/vulkan.h>
#include <glm/vec3.hpp>
#include <fastgltf/core.hpp>

#include <vector>
#include <string_view>
#include <fstream>
#include <optional>
#include <unordered_map>
#include <memory>

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
VkRenderingInfo makeRenderingInfo(VkExtent2D renderExtent, uint32_t colorAttachmentCount,
    VkRenderingAttachmentInfo* colorAttachments, VkRenderingAttachmentInfo* depthAttachment);
VkBufferMemoryBarrier makeBufferMemoryBarrier(VkBuffer buffer, uint32_t queueIndex);
VkImageMemoryBarrier makeImageMemoryBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
    VkImageLayout oldLayout, VkImageLayout newLayout, VkImageAspectFlags aspectMask);

void addVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec3& tangent, const glm::vec4& color,
    const glm::vec2& texCoord, std::vector<uint32_t>& indices, std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap);
void addTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec4& color,
    const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
    std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap);
void addQuad(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4, const glm::vec4& color,
    const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
    std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap);
const IdType createCubeMeshToBank(MeshBank<NormalVertex>* meshBank, IdType materialId, IdType materialInstanceId);
void loadMeshFromGltf(MeshBank<NormalVertex>* meshBank, MaterialProvider* materialBank, fastgltf::Asset& gltfAsset);

//  find the largest power of 2 that's less than val
uint32_t findPreviousPow2(uint32_t val);

} // namespace Bunny::Render