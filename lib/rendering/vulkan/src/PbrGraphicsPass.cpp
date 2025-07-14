#include "PbrGraphicsPass.h"

#include "Error.h"

#include <cassert>

namespace Bunny::Render
{
PbrGraphicsPass::PbrGraphicsPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer),
      mMaterialBank(materialBank),
      mMeshBank(meshBank)
{
}

PbrGraphicsPass::~PbrGraphicsPass()
{
    cleanup();
}

BunnyResult PbrGraphicsPass::initializePass()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptors())
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initPipeline())
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDataAndResources())

    return BUNNY_HAPPY;
}

void PbrGraphicsPass::cleanup()
{
    mDeletionStack.Flush();
}

BunnyResult PbrGraphicsPass::initPipeline()
{
    return BUNNY_HAPPY;
}
BunnyResult PbrGraphicsPass::initDescriptors()
{
    return BUNNY_HAPPY;
}
BunnyResult PbrGraphicsPass::initDataAndResources()
{
    return BUNNY_HAPPY;
}
} // namespace Bunny::Render
