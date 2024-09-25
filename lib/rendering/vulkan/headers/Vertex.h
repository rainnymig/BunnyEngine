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
    glm::vec2 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
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
} // namespace Bunny::Render