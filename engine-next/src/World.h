#pragma once

#include "Transform.h"
#include "Fundamentals.h"
#include "BunnyResult.h"
#include "MeshBank.h"
#include "Vertex.h"

#include <entt/entt.hpp>

#include <string_view>

namespace Bunny::Render
{
    class VulkanRenderResources;
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

    class World
    {
    public:
        entt::registry mEntityRegistry;
    };

    class WorldLoader
    {
    public:
        BunnyResult loadGltfToWorld(std::string_view filePath, World outWorld);

    private:
        const Render::VulkanRenderResources* mVulkanResources;
        Render::MeshBank<Render::NormalVertex>* mMeshBank;
    };

    class WorldPhysicsSystem
    {

    };

    class WorldUpdateSystem
    {

    };
} // namespace Bunny::Engine
