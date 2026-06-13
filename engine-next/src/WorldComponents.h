#pragma once

#include "Fundamentals.h"
#include "Transform.h"
#include "Light.h"
#include "Camera.h"

#include <entt/entt.hpp>

namespace Bunny
{
struct TransformComponent
{
    Transform mTransform;
};

struct MeshComponent
{
    IdType mMeshId;
    IdType mMaterialId;
};

struct HierarchyComponent
{
    entt::entity mParent{entt::null};
};

struct DirectionLightComponent
{
    DirectionalLight mLight;
};

struct PbrLightComponent
{
    PbrLight mLight;
};

struct CameraComponent
{
    Camera mCamera;
};

struct PbrCameraComponent
{
    PhysicalCamera mCamera;
};
} // namespace Bunny
