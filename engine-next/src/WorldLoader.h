#pragma once

#include "BunnyResult.h"
#include "MeshBank.h"
#include "Vertex.h"
#include "World.h"

namespace Bunny::Render
{
class VulkanRenderResources;
class PbrMaterialBank;
class TextureBank;
} // namespace Bunny::Render

namespace Bunny::Engine
{
class WorldLoader
{
  public:
    WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::PbrMaterialBank* pbrMaterialBank,
        Render::MeshBank<Render::NormalVertex>* meshBank, Render::TextureBank* textureBank);

    BunnyResult loadPbrTestWorldWithGltfMeshes(std::string_view filePath, World& outWorld);

  private:
    void postLoad(World& outWorld);

    const Render::VulkanRenderResources* mVulkanResources;
    Render::PbrMaterialBank* mPbrMaterialBank;
    Render::MeshBank<Render::NormalVertex>* mMeshBank;
    Render::TextureBank* mTextureBank;
};
} // namespace Bunny::Engine
