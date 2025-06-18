#pragma once

#include "Fundamentals.h"
#include "Transform.h"
#include "Light.h"
#include "Camera.h"

#include <entt/entt.hpp>

namespace Bunny::Engine
{
struct TransformComponent
{
    Base::Transform mTransform;
};

struct MeshComponent
{
    Render::IdType mMeshId;
    Render::IdType mMaterialId;
};

struct HierarchyComponent
{
    entt::entity mParent{entt::null};
};

struct DirectionLightComponent
{
    Render::DirectionalLight mLight;
};

struct PbrLightComponent
{
    Render::PbrLight mLight;
};

struct CameraComponent
{
    Render::Camera mCamera;
};

struct PbrCameraComponent
{
    Render::PhysicalCamera mCamera;
};
} // namespace Bunny::Engine
