#include "Helper.h"

#include "ErrorCheck.h"

namespace Bunny::Render
{
VkCommandPoolCreateInfo makeCommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags)
{
    VkCommandPoolCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.pNext = nullptr;
    info.queueFamilyIndex = queueFamilyIndex;
    info.flags = flags;
    return info;
}

VkCommandBufferAllocateInfo makeCommandBufferAllocateInfo(VkCommandPool commandPool, uint32_t bufferCount)
{
    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext = nullptr;
    info.commandPool = commandPool;
    info.commandBufferCount = bufferCount;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}

VkCommandBufferSubmitInfo makeCommandBufferSubmitInfo(VkCommandBuffer commandBuffer)
{
    VkCommandBufferSubmitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext = nullptr;
    info.commandBuffer = commandBuffer;
    info.deviceMask = 0;

    return info;
}

VkSubmitInfo2 makeSubmitInfo2(VkCommandBufferSubmitInfo* cmdBufferSubmit, VkSemaphoreSubmitInfo* signalSemaphoreInfo,
    VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
    VkSubmitInfo2 info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext = nullptr;

    info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos = waitSemaphoreInfo;

    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos = signalSemaphoreInfo;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmdBufferSubmit;

    return info;
}

VkPipelineShaderStageCreateInfo makeShaderStageCreateInfo(
    VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char* entryPoint)
{
    VkPipelineShaderStageCreateInfo info{.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext = nullptr,
        .stage = stage,
        .module = shaderModule,
        .pName = entryPoint};
    return info;
}

VkRenderingAttachmentInfo makeColorAttachmentInfo(VkImageView view, VkClearValue* clearValue, VkImageLayout layout)
{
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.pNext = nullptr;

    colorAttachment.imageView = view;
    colorAttachment.imageLayout = layout;
    colorAttachment.loadOp = clearValue ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    if (clearValue)
    {
        colorAttachment.clearValue = *clearValue;
    }

    return colorAttachment;
}

VkRenderingAttachmentInfo makeDepthAttachmentInfo(VkImageView view, VkImageLayout layout)
{
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.pNext = nullptr;

    depthAttachment.imageView = view;
    depthAttachment.imageLayout = layout;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil.depth = 1.f;

    return depthAttachment;
}

VkRenderingInfo makeRenderingInfo(
    VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment)
{
    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;

    renderInfo.renderArea = VkRect2D{
        VkOffset2D{0, 0},
        renderExtent
    };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = colorAttachment;
    renderInfo.pDepthAttachment = depthAttachment;
    renderInfo.pStencilAttachment = nullptr;

    return renderInfo;
}

bool hasStencilComponent(VkFormat format)
{
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void transitionImageLayout(
    VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;

    VkImageSubresourceRange range{
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format))
        {
            range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange = range;
    barrier.image = image;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

void addVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec4& color, const glm::vec2& texCoord,
    std::vector<uint32_t>& indices, std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap)
{
    NormalVertex newVertex{.mPosition = position, .mNormal = normal, .mColor = color, .mTexCoord = texCoord};

    const auto verIdxPair = vertexToIndexMap.find(newVertex);
    if (verIdxPair != vertexToIndexMap.end())
    {
        indices.push_back(verIdxPair->second);
    }
    else
    {
        uint32_t newIndex = vertices.size();
        vertices.emplace_back(newVertex);
        indices.emplace_back(newIndex);
        vertexToIndexMap[newVertex] = newIndex;
    }
}

void addTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec4& color,
    const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
    std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap)
{
    //  p1 - - p3 - u
    //  |     /
    //  |   /
    //  | /
    //  p2
    //  |
    //  v

    glm::vec3 v12 = p2 - p1;
    glm::vec3 v13 = p3 - p1;
    // glm::vec3 normal = glm::normalize(glm::cross(v12, v13));
    glm::vec3 normal = glm::normalize(glm::cross(v13, v12)); //  revisit this direction later

    glm::vec2 tex1 = texCoordBase;
    glm::vec2 tex2 = texCoordBase + glm::vec2{0, scale};
    glm::vec2 tex3 = texCoordBase + glm::vec2{scale, 0};

    addVertex(p1, normal, color, tex1, indices, vertices, vertexToIndexMap);
    addVertex(p2, normal, color, tex2, indices, vertices, vertexToIndexMap);
    addVertex(p3, normal, color, tex3, indices, vertices, vertexToIndexMap);
}

void addQuad(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4, const glm::vec4& color,
    const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
    std::vector<NormalVertex>& vertices,
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap)
{
    //  p1 - - p4 - u
    //  |     / |
    //  |   /   |
    //  | /     |
    //  p2 - - p3
    //  |
    //  v

    glm::vec3 v12 = p2 - p1;
    glm::vec3 v14 = p4 - p1;
    // glm::vec3 normal = glm::normalize(glm::cross(v12, v14));
    glm::vec3 normal = glm::normalize(glm::cross(v14, v12)); //  revisit this direction later

    glm::vec2 tex1 = texCoordBase;
    glm::vec2 tex2 = texCoordBase + glm::vec2{0, scale};
    glm::vec2 tex3 = texCoordBase + glm::vec2{scale, scale};
    glm::vec2 tex4 = texCoordBase + glm::vec2{scale, 0};

    addVertex(p1, normal, color, tex1, indices, vertices, vertexToIndexMap);
    addVertex(p2, normal, color, tex2, indices, vertices, vertexToIndexMap);
    addVertex(p4, normal, color, tex4, indices, vertices, vertexToIndexMap);

    addVertex(p4, normal, color, tex4, indices, vertices, vertexToIndexMap);
    addVertex(p2, normal, color, tex2, indices, vertices, vertexToIndexMap);
    addVertex(p3, normal, color, tex3, indices, vertices, vertexToIndexMap);
}

const IdType createCubeMeshToBank(MeshBank<NormalVertex>* meshBank, IdType materialId)
{
    MeshLite newMesh;
    newMesh.mName = "Cube";
    IdType meshId = std::hash<std::string>{}(newMesh.mName);
    newMesh.mId = meshId;

    SurfaceLite cubeSurface;
    cubeSurface.mFirstIndex = 0;
    cubeSurface.mMaterialInstanceId = materialId;

    std::vector<uint32_t> indices;
    std::vector<NormalVertex> vertices;
    std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash> vertexToIndexMap;

    constexpr glm::vec4 red{1.0f, 0.0, 0.0, 1.0};
    constexpr glm::vec4 green{0.0f, 1.0, 0.0, 1.0};
    constexpr glm::vec4 blue{0.0f, 0.0, 1.0, 1.0};
    constexpr glm::vec4 yellow{1.0f, 1.0, 0.0, 1.0};
    constexpr glm::vec4 fuchsia{1.0f, 0.0, 1.0, 1.0};
    constexpr glm::vec4 aqua{0.0f, 1.0, 1.0, 1.0};
    constexpr glm::vec4 white{1.0f, 1.0, 1.0, 1.0};
    //  front   -z
    addQuad({-0.5, 0.5, -0.5}, {-0.5, -0.5, -0.5}, {0.5, -0.5, -0.5}, {0.5, 0.5, -0.5}, aqua, {0, 0}, 1, indices,
        vertices, vertexToIndexMap);
    //  right   +x
    addQuad({0.5, 0.5, -0.5}, {0.5, -0.5, -0.5}, {0.5, -0.5, 0.5}, {0.5, 0.5, 0.5}, red, {0, 0}, 1, indices, vertices,
        vertexToIndexMap);
    //  up      +y
    addQuad({-0.5, 0.5, 0.5}, {-0.5, 0.5, -0.5}, {0.5, 0.5, -0.5}, {0.5, 0.5, 0.5}, green, {0, 0}, 1, indices, vertices,
        vertexToIndexMap);
    //  left    -x
    addQuad({-0.5, 0.5, 0.5}, {-0.5, -0.5, 0.5}, {-0.5, -0.5, -0.5}, {-0.5, 0.5, -0.5}, yellow, {0, 0}, 1, indices,
        vertices, vertexToIndexMap);
    //  bottom  -y
    addQuad({-0.5, -0.5, -0.5}, {-0.5, -0.5, 0.5}, {0.5, -0.5, 0.5}, {0.5, -0.5, -0.5}, fuchsia, {0, 0}, 1, indices,
        vertices, vertexToIndexMap);
    //  back    +z
    addQuad({0.5, 0.5, 0.5}, {0.5, -0.5, 0.5}, {-0.5, -0.5, 0.5}, {-0.5, 0.5, 0.5}, blue, {0, 0}, 1, indices, vertices,
        vertexToIndexMap);

    cubeSurface.mIndexCount = indices.size();
    newMesh.mSurfaces.push_back(cubeSurface);
    newMesh.mBounds.mCenter = glm::vec3{0, 0, 0};
    newMesh.mBounds.mRadius = 0.9f; //  the exact radius should be sqrt(3)/2;

    meshBank->addMesh(vertices, indices, newMesh);

    return meshId;
}

} // namespace Bunny::Render