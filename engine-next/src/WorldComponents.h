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
};

struct HierarchyComponent
{
    entt::entity mParent{entt::null};
};

struct DirectionLightComponent
{
    Render::DirectionalLight mLight;
};

struct CameraComponent
{
    Render::Camera mCamera;
};
} // namespace Bunny::Engine
