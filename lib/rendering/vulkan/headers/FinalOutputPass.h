#pragma once

#include "PbrGraphicsPass.h"
#include "Fundamentals.h"

namespace Bunny::Render
{
class FinalOutputPass : public PbrGraphicsPass
{
  public:
    void draw() const override;

  private:
    struct FrameData
    {
        VkDescriptorSet mInputDescSet;

        const AllocatedImage* mCloudTexture = nullptr;
        const AllocatedImage* mFogShadowTexture = nullptr;
        const AllocatedImage* mRenderedSceneTexture = nullptr;
    };
};
} // namespace Bunny::Render
