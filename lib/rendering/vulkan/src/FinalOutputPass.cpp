#include "FinalOutputPass.h"

namespace Bunny::Render
{
FinalOutputPass::FinalOutputPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    std::string_view vertShader, std::string_view fragShader)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mVertexShaderPath(vertShader),
      mFragmentShaderPath(fragShader)
{
}

void FinalOutputPass::draw() const
{
}

void FinalOutputPass::updateInputTextures(const AllocatedImage* cloudTexture, const AllocatedImage* fogShadowTexture,
    const AllocatedImage* renderedSceneTexture)
{
}

BunnyResult FinalOutputPass::initPipeline()
{
    return BUNNY_HAPPY;
}

BunnyResult FinalOutputPass::initDescriptors()
{
    return BUNNY_HAPPY;
}

BunnyResult FinalOutputPass::initDescriptorLayouts()
{
    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
