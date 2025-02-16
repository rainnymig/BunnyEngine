#include "BaseVulkanRenderer.h"

#include <Mesh.h>

namespace Bunny::Render
{
void BaseVulkanRenderer::createAndMapMeshBuffers(
    Mesh* mesh, std::span<NormalVertex> vertices, std::span<uint32_t> indices)
{
}

AllocatedBuffer BaseVulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage,
    VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage) const
{
    return AllocatedBuffer();
}

void BaseVulkanRenderer::destroyBuffer(const AllocatedBuffer& buffer) const
{
}

uint32_t BaseVulkanRenderer::getCurrentFrameId() const
{
    return 0;
}

BasicBlinnPhongMaterial* BaseVulkanRenderer::getMaterial() const
{
    return nullptr;
}

} // namespace Bunny::Render
