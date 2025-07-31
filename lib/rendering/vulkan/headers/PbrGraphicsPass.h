#pragma once

#include "Fundamentals.h"
#include "BunnyResult.h"
#include "FunctionStack.h"
#include "MeshBank.h"
#include "Vertex.h"

#include <volk.h>

#include <string_view>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;
class PbrMaterialBank;

class PbrGraphicsPass
{
  public:
    PbrGraphicsPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank);
    virtual ~PbrGraphicsPass();

    BunnyResult initializePass();
    virtual void draw() const = 0;
    virtual void cleanup();

  protected:
    virtual BunnyResult initPipeline();
    virtual BunnyResult initDescriptors();
    virtual BunnyResult initDataAndResources();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
    const PbrMaterialBank* mMaterialBank;
    const MeshBank<NormalVertex>* mMeshBank;

    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;

    Base::FunctionStack<> mDeletionStack;
    bool mIsInitialized = false;
};
} // namespace Bunny::Render
