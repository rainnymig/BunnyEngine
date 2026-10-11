#pragma once

#include "BunnyResult.h"
#include "MeshBank.h"
#include "Vertex.h"
#include "World.h"

#include <fastgltf/tools.hpp>

namespace Bunny
{
class PbrMaterialBank;
class TextureBank;
} // namespace Bunny

namespace Bunny
{
class WorldLoader
{
  public:
    WorldLoader(PbrMaterialBank* pbrMaterialBank, MeshBank<NormalVertex>* meshBank, TextureBank* textureBank);

    BunnyResult loadPbrTestWorldWithGltfMeshes(std::string_view filePath, World& outWorld);

  private:
    void postLoad(World& outWorld);
    void loadWorldStructure(fastgltf::Asset& gltfAsset, World& outWorld);

    PbrMaterialBank* mPbrMaterialBank;
    MeshBank<NormalVertex>* mMeshBank;
    TextureBank* mTextureBank;
};
} // namespace Bunny
