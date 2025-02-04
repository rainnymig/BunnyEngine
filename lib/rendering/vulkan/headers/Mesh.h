#pragma once

#include "Fundamentals.h"
#include "Material.h"

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <memory>

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
    std::unordered_map<size_t, std::unique_ptr<Mesh>> mMeshes;
};

} // namespace Bunny::Render