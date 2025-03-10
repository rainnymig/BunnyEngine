#pragma once

#include "Transform.h"
#include "Fundamentals.h"
#include "BunnyResult.h"
#include "MeshBank.h"
#include "Vertex.h"
#include "Light.h"
#include "Camera.h"

#include <entt/entt.hpp>

#include <string_view>

namespace Bunny::Render
{
    class VulkanRenderResources;
    class MaterialBank;
}

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

    class World
    {
    public:
        entt::registry mEntityRegistry;
    };

    class WorldLoader
    {
    public:
        BunnyResult loadGltfToWorld(std::string_view filePath, World& outWorld);
        BunnyResult loadTestWorld(World& outWorld);

    private:
        const Render::VulkanRenderResources* mVulkanResources;
        Render::MaterialBank* mMaterialBank;
        Render::MeshBank<Render::NormalVertex>* mMeshBank;
    };

    class WorldPhysicsSystem
    {

    };

    class WorldUpdateSystem
    {

    };
} // namespace Bunny::Engine
