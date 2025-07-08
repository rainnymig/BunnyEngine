#pragma once

#include "PbrGraphicsPass.h"

namespace Bunny::Render
{
class RaytracingShadowPass : public PbrGraphicsPass
{
  public:
    RaytracingShadowPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank, std::string_view raygenShader,
        std::string_view closestHitShader, std::string_view missShader);

    virtual void draw() const override;

  protected:
    virtual BunnyResult initPipeline() override;

  private:
    using super = PbrGraphicsPass;

    BunnyResult buildPipelineLayout();

    std::string_view mRaygenShaderPath;
    std::string_view mClosestHitShaderPath;
    std::string_view mMissShaderPath;
};

} // namespace Bunny::Render
