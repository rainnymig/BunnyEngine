#pragma once

#include <volk.h>
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

struct ScreenQuadVertex
{
    glm::vec3 pos;
    glm::vec2 texCoord;

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(ScreenQuadVertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(ScreenQuadVertex, texCoord);

        return attributeDescriptions;
    }
};

struct NormalVertex
{
    glm::vec3 mPosition;
    glm::vec3 mNormal;
    glm::vec3 mTangent;
    glm::vec4 mColor;
    glm::vec2 mTexCoord;

    struct Hash
    {
        size_t operator()(const NormalVertex& v) const;
    };

    bool operator==(const NormalVertex& rhs) const { return memcmp(this, &rhs, sizeof(NormalVertex)) == 0; }

    static constexpr std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

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
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(NormalVertex, mTangent);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(NormalVertex, mColor);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(NormalVertex, mTexCoord);

        return attributeDescriptions;
    }
};
} // namespace Bunny::Render