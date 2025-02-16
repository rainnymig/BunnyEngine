#pragma once

#include "Fundamentals.h"
#include "Material.h"
#include "BaseVulkanRenderer.h"

#include <vulkan/vulkan.h>
#include <string>
#include <unordered_map>
#include <memory>

namespace Bunny::Render
{

struct Surface
{
    uint32_t mStartIndex;
    uint32_t mIndexCount;
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
    explicit MeshAssetsBank(const BaseVulkanRenderer* renderer) : mRenderer(renderer) {}
    void destroyBank();

    std::unordered_map<size_t, std::unique_ptr<Mesh>> mMeshes;
    const BaseVulkanRenderer* mRenderer = nullptr;
};

} // namespace Bunny::Render