#pragma once

#include "Renderer.h"
#include "Vertex.h"

#include <span>

namespace Bunny::Render
{
class BaseVulkanRenderer : public Renderer
{
  public:
    virtual void createAndMapMeshBuffers(
        class Mesh* mesh, std::span<NormalVertex> vertices, std::span<uint32_t> indices) = 0;
};
} // namespace Bunny::Render
