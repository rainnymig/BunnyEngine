#pragma once

#include "BunnyResult.h"
#include "Vertex.h"
#include "MeshBank.h"

#include <memory>

namespace Bunny
{

class Engine;

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class TextureBank;
class PbrMaterialBank;
class WorldRenderDataTranslator;

class GraphicsSystem
{
  public:
    BunnyResult initialize(Engine& engine);
    BunnyResult draw();
    BunnyResult shutdown();

  private:
    std::unique_ptr<VulkanRenderResources> mRenderResources;
    std::unique_ptr<VulkanGraphicsRenderer> mRenderer;

    std::unique_ptr<TextureBank> mTextureBank;
    std::unique_ptr<MeshBank<NormalVertex>> mMeshBank;
    std::unique_ptr<PbrMaterialBank> mPbrMaterialBank;

    std::unique_ptr<WorldRenderDataTranslator> mWorldTranslator;

    Engine* mEngine;
};
} // namespace Bunny
