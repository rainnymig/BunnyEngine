#pragma once

#include <glm/vec3.hpp>

namespace Bunny::Render
{

enum class LightType : uint32_t
{
    Directional = 0,
    Point = 1
};

struct DirectionalLight
{
    glm::vec3 mDirection;
    float mPadding1;
    glm::vec3 mColor;
    float mPadding2;
};

struct PbrLight
{
    glm::vec3 mDirOrPos;
    float mIntensity; //  dir light - illuminance (lux or lumen/m2); point/spot light - luminous power (lumen)
    glm::vec3 mColor;
    float mInfluenceRadius;
    float mInnerAngle;
    float mOuterAngle;
    LightType mType = LightType::Directional;
    float mPadding;
};
} // namespace Bunny::Render
