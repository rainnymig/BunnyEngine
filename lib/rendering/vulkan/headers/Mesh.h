#pragma once

#include <vulkan/vulkan.h>

#include <string_view>

namespace Bunny::Render
{
class Mesh
{
  public:
    struct RenderBuffer
    {
        VkBuffer mVertexBuffer;
        VkBuffer mIndexBuffer;
    };

    ~Mesh();

    RenderBuffer getRenderBuffer() const { return mRenderBuffer; }

  protected:
    void destroyBuffers();

    RenderBuffer mRenderBuffer{nullptr, nullptr};
    VkDeviceMemory mVertexBufferMemory;
    VkDeviceMemory mIndexBufferMemory;
    VkDevice mDevice;
};

class ModelMesh : public Mesh
{
  public:
    bool loadFromFile(std::string_view filePath, VkDevice device, VkPhysicalDevice physicalDevice);
};

} // namespace Bunny::Render