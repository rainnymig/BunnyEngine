#pragma once

#include "Light.h"
#include "Fundamentals.h"

#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Bunny::Render
{
struct SceneData
{
    glm::mat4x4 mViewMatrix;
    glm::mat4x4 mProjMatrix;
    glm::mat4x4 mViewProjMatrix;
};

struct LightData
{
    static constexpr size_t MAX_LIGHT_COUNT = 8;
    glm::vec3 mCameraPos;
    uint32_t mLightCount;
    DirectionalLight mLights[MAX_LIGHT_COUNT];
};

struct ObjectData
{
    glm::mat4 model;
    glm::mat4 invTransModel;
    glm::vec3 scale;
    IdType meshId;
};

} // namespace Bunny::Render
