#pragma once

#include "Fundamentals.h"
#include "Material.h"

#include <vulkan/vulkan.h>
#include <string>

namespace Bunny::Render
{

struct Surface
{
    uint32_t mStartIndex;
    uint32_t mCount;
    MaterialInstance mMaterial;
};

struct Mesh
{
    std::string mName;
    std::vector<Surface> mSurfaces;
    AllocatedBuffer mVertexBuffer;
    AllocatedBuffer mIndexBuffer;
};

struct MeshAssetsBank
{
    std::vector<Mesh> mMeshes;
    std::vector<MaterialInstance> mMaterials;
};

} // namespace Bunny::Render