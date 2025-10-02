#pragma once

#include "Descriptor.h"
#include "Fundamentals.h"

#include "BunnyResult.h"
#include "BunnyGuard.h"

#include <volk.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <string_view>
#include <memory>
#include <array>
#include <string>

namespace Bunny::Render
{
struct PbrMaterialParameters
{
    glm::vec4 mBaseColor;
    glm::vec4 mEmissiveColor;
    float mMetallic;
    float mRoughness;
    float mReflectance;
    float mAmbientOcclusion;
    IdType mColorTexId;
    IdType mNormalTexId;
    IdType mEmissiveTexId;
    IdType mMetRouRflAmbTexId;
};
} // namespace Bunny::Render
