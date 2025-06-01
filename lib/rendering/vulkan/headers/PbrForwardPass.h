#pragma once

#include "PbrGraphicsPass.h"

namespace Bunny::Render
{
class PbrForwardPass : public PbrGraphicsPass
{
  public:
    virtual void draw() const override;

  protected:
    virtual BunnyResult initPipeline() override;
};
} // namespace Bunny::Render
