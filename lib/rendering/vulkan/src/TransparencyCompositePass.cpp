#include "TransparencyCompositePass.h"

#include "VulkanRenderResources.h"
#include "VulkanGraphicsRenderer.h"

namespace Bunny::Render
{
TransparencyCompositePass::TransparencyCompositePass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
    std::string_view vertShader, std::string_view fragShader)
    : super(vulkanResources, renderer, materialBank, meshBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void TransparencyCompositePass::draw() const
{
}

void TransparencyCompositePass::setSceneRenderTarget(const AllocatedImage* sceneRenderTarget)
{
}

void TransparencyCompositePass::linkTransparentImages(
    const std::array<const AllocatedImage*, MAX_FRAMES_IN_FLIGHT>& accumImages,
    const std::array<const AllocatedImage*, MAX_FRAMES_IN_FLIGHT>& revealImages)
{
}

BunnyResult TransparencyCompositePass::initPipeline()
{
    return BUNNY_HAPPY;
}

BunnyResult TransparencyCompositePass::initDescriptors()
{
    return BUNNY_HAPPY;
}

BunnyResult TransparencyCompositePass::initDataAndResources()
{
    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
