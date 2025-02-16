#pragma once

#include "Renderer.h"
#include "Vertex.h"
#include "Material.h"
#include "Fundamentals.h"

#include <span>

namespace Bunny::Render
{
class BaseVulkanRenderer : public Renderer
{
  public:
    virtual void createAndMapMeshBuffers(
        class Mesh* mesh, std::span<NormalVertex> vertices, std::span<uint32_t> indices);

    virtual AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage,
        VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage) const;
    virtual void destroyBuffer(const AllocatedBuffer& buffer) const;

    virtual uint32_t getCurrentFrameId() const;

    //  temp
    virtual BasicBlinnPhongMaterial* getMaterial() const;
};

} // namespace Bunny::Render
