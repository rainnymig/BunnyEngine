#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"
#include "PbrGraphicsPass.h"

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class TextureBank;
class Camera;

class OceanPass : PbrGraphicsPass
{
  public:
    void draw() const override;

  protected:
    BunnyResult initPipeline() override;
    BunnyResult initDescriptors() override;
    BunnyResult initDataAndResources() override;

  private:
    using super = PbrGraphicsPass;
};
} // namespace Bunny::Render
