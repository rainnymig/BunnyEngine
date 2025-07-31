#pragma once

#include "Light.h"
#include "Fundamentals.h"

#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <volk.h>

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
    uint32_t vertexOffset; //  from mesh data: put the vertex offset of the mesh here
                           //  so that easier to find the vertices of a primitive in the ray tracing shaders
    uint32_t firstIndex;   //  from mesh data: also for rt pipeline
                           //  the idx of the first index of the mesh in the shared index buffer
    uint32_t mPadding;
};

struct PbrLightData
{
    PbrLight mLights[MAX_LIGHT_COUNT];
    uint32_t mLightCount;
};

struct PbrCameraData
{
    glm::mat4 mViewProjMat;
    glm::mat4 mInverseView;
    glm::mat4 mInverseProj;
    glm::vec3 mPosition;
    float mExposure;
    glm::vec2 mResolution;
    float mPadding1;
    float mPadding2;
};

struct VertexIndexBufferData
{
    VkDeviceAddress mVertexBufferAddress;
    VkDeviceAddress mIndexBufferAddress;
};

} // namespace Bunny::Render
