#include "PbrGraphicsPass.h"

#include "Error.h"

#include <cassert>

namespace Bunny::Render
{
PbrGraphicsPass::PbrGraphicsPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank, std::string_view vertShader,
    std::string_view fragShader)
    : mVulkanResources(vulkanResources),
      mRenderer(renderer),
      mMaterialBank(materialBank),
      mMeshBank(meshBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

PbrGraphicsPass::~PbrGraphicsPass()
{
    cleanup();
}

BunnyResult PbrGraphicsPass::initializePass()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initPipeline())

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
} // namespace Bunny::Render
