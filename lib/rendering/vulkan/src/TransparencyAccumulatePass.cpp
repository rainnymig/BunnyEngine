#include "TransparencyAccumulatePass.h"

namespace Bunny::Render
{
Render::TransparencyAccumulatePass::TransparencyAccumulatePass(const VulkanRenderResources* vulkanResources,
    const VulkanGraphicsRenderer* renderer, const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank,
    std::string_view vertShader, std::string_view fragShader)
    : super(vulkanResources, renderer, materialBank, meshBank),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void Render::TransparencyAccumulatePass::draw() const
{
}

void Render::TransparencyAccumulatePass::linkWorldData(
    const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData)
{
}

void Render::TransparencyAccumulatePass::linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize)
{
}

void Render::TransparencyAccumulatePass::linkShadowData(std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> shadowImageViews)
{
}

BunnyResult Render::TransparencyAccumulatePass::initPipeline()
{
    return BUNNY_HAPPY;
}

BunnyResult Render::TransparencyAccumulatePass::initDescriptors()
{
    return BUNNY_HAPPY;
}

BunnyResult Render::TransparencyAccumulatePass::initDataAndResources()
{
    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
