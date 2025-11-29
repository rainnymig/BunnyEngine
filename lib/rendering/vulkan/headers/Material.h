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
    glm::vec4 mBaseColor{1, 1, 1, 1};
    glm::vec4 mEmissiveColor{0, 0, 0, 0};
    float mMetallic = 1;
    float mRoughness = 1;
    float mReflectance = 0.5f;
    float mAmbientOcclusion = 0;
    IdType mColorTexId = BUNNY_INVALID_ID;
    IdType mNormalTexId = BUNNY_INVALID_ID;
    IdType mEmissiveTexId = BUNNY_INVALID_ID;
    IdType mMetalRoughnessTexId = BUNNY_INVALID_ID;
};
} // namespace Bunny::Render
