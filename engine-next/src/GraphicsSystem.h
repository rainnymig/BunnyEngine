#pragma once

#include "BunnyResult.h"

#include <memory>

namespace Bunny
{

class Engine;

class VulkanRenderResources;
class VulkanGraphicsRenderer;

class GraphicsSystem
{
  public:
    BunnyResult initialize(Engine& engine);
    BunnyResult draw();
    BunnyResult shutdown();

  private:
    std::unique_ptr<VulkanRenderResources> mRenderResources;
    std::unique_ptr<VulkanGraphicsRenderer> mRenderer;

    Engine* mEngine;
};
} // namespace Bunny
