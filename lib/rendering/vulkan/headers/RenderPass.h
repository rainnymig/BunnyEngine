#pragma once

#include "RenderJob.h"
#include "Vertex.h"
#include "MeshBank.h"

#include <vector>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class MaterialBank;

class ForwardPass
{
  public:
    void initializePass();
    void renderBatch(const RenderBatch& batch);
    void cleanup();

  private:
    VulkanRenderResources* mVulkanResources;
    VulkanGraphicsRenderer* mRenderer;
    const MaterialBank* mMaterialBank;
    const MeshBank<NormalVertex>* mMeshBank;
};

} // namespace Bunny::Render
