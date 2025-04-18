#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

namespace Bunny::Render
{

enum class VertexInputRate
{
    Vertex,
    Instance
};

template <typename VertexType>
constexpr VkVertexInputBindingDescription getBindingDescription(uint32_t bindingIndex, VertexInputRate inputRate)
{
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = bindingIndex;
    bindingDescription.stride = sizeof(VertexType);
    if (inputRate == VertexInputRate::Vertex)
    {
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    }
    else
    {
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    }
    return bindingDescription;
}

struct BasicVertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(BasicVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(BasicVertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(BasicVertex, texCoord);

        return attributeDescriptions;
    }
};

struct NormalVertex
{
    glm::vec3 mPosition;
    glm::vec3 mNormal;
    glm::vec4 mColor;
    glm::vec2 mTexCoord;

    struct Hash
    {
        size_t operator()(const NormalVertex& v) const;
    };

    bool operator==(const NormalVertex& rhs) const { return memcmp(this, &rhs, sizeof(NormalVertex)) == 0; }

    static constexpr std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(NormalVertex, mPosition);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(NormalVertex, mNormal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(NormalVertex, mColor);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(NormalVertex, mTexCoord);

        return attributeDescriptions;
    }
};
} // namespace Bunny::Render