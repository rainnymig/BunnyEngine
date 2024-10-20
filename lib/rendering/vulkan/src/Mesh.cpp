#include "Mesh.h"

#include "Helper.h"
#include "Vertex.h"

#include <tiny_obj_loader.h>

namespace Bunny::Render
{
Mesh::~Mesh()
{
    destroyBuffers();
}
void Mesh::destroyBuffers()
{
    if (mDevice != nullptr)
    {
        if (mRenderBuffer.mVertexBuffer != nullptr)
        {
            vkDestroyBuffer(mDevice, mRenderBuffer.mVertexBuffer, nullptr);
            mRenderBuffer.mVertexBuffer = nullptr;
        }
        if (mRenderBuffer.mIndexBuffer != nullptr)
        {
            vkDestroyBuffer(mDevice, mRenderBuffer.mIndexBuffer, nullptr);
            mRenderBuffer.mIndexBuffer = nullptr;
        }
    }
}
bool ModelMesh::loadFromFile(std::string_view filePath, VkDevice device, VkPhysicalDevice physicalDevice)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filePath.data()))
    {
        return false;
    }

    std::vector<BasicVertex> modelVertices = {};
    std::vector<uint32_t> modelIndecies = {};

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            BasicVertex vertex{};

            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]};
            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
            vertex.color = {1.0f, 1.0f, 1.0f};

            modelVertices.push_back(vertex);
            modelIndecies.push_back(modelIndecies.size());
        }
    }

    //  create vertex buffer
    {
        VkDeviceSize bufferSize = sizeof(modelVertices[0]) * modelVertices.size();
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mRenderBuffer.mVertexBuffer, mVertexBufferMemory);
        void* data;
        vkMapMemory(mDevice, mVertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, modelVertices.data(), (size_t)bufferSize);
        vkUnmapMemory(mDevice, mVertexBufferMemory);
    }

    //  create index buffer
    {
        VkDeviceSize bufferSize = sizeof(modelIndecies[0]) * modelIndecies.size();
        createBuffer(device, physicalDevice, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mRenderBuffer.mIndexBuffer, mIndexBufferMemory);
        void* data;
        vkMapMemory(mDevice, mIndexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, modelIndecies.data(), (size_t)bufferSize);
        vkUnmapMemory(mDevice, mIndexBufferMemory);
    }
    return true;
}
} // namespace Bunny::Render