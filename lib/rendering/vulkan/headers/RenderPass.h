#pragma once

#include "RenderJob.h"
#include "MaterialBank.h"
#include "Vertex.h"

#include <vector>

namespace Bunny::Render
{

class VulkanRenderResources;
class MaterialBank;

class ForwardPass
{
  public:
    void initializePass();
    void renderBatch(const std::vector<RenderBatch>& batches);
    void cleanup();

  private:
    VulkanRenderResources* mVulkanResources;
    const MaterialBank* mMaterialBank;
    const MeshBank<NormalVertex>* mMeshBank;
};

} // namespace Bunny::Render
