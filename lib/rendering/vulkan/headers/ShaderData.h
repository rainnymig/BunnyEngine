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

static constexpr size_t MAX_LIGHT_COUNT = 8;

struct LightData
{
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
    IdType materialId;
};

struct PbrLightData
{
    PbrLight mLights[MAX_LIGHT_COUNT];
    uint32_t mLightCount;
};

struct PbrCameraData
{
    glm::mat4 mViewProjMat;
    glm::vec3 mPosition;
    float mExposure;
};

} // namespace Bunny::Render
