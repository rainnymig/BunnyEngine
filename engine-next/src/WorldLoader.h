#pragma once

#include "BunnyResult.h"
#include "MeshBank.h"
#include "Vertex.h"
#include "World.h"

namespace Bunny::Render
{
class VulkanRenderResources;
class MaterialBank;
class PbrMaterialBank;
} // namespace Bunny::Render

namespace Bunny::Engine
{
class WorldLoader
{
  public:
    WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::MaterialBank* materialBank,
        Render::MeshBank<Render::NormalVertex>* meshBank);
    WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::PbrMaterialBank* pbrMaterialBank,
        Render::MeshBank<Render::NormalVertex>* meshBank);

    BunnyResult loadGltfToWorld(std::string_view filePath, World& outWorld);
    BunnyResult loadPbrTestWorldWithGltfMeshes(std::string_view filePath, World& outWorld);
    BunnyResult loadTestWorld(World& outWorld);

  private:
    void postLoad(World& outWorld);

    const Render::VulkanRenderResources* mVulkanResources;
    Render::MaterialBank* mMaterialBank;
    Render::PbrMaterialBank* mPbrMaterialBank;
    Render::MeshBank<Render::NormalVertex>* mMeshBank;
};
} // namespace Bunny::Engine
