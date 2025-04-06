#pragma once

#include "Transform.h"
#include "Fundamentals.h"
#include "BunnyResult.h"
#include "MeshBank.h"
#include "Vertex.h"
#include "Light.h"
#include "Camera.h"
#include "ShaderData.h"
#include "RenderJob.h"

#include <entt/entt.hpp>

#include <string_view>
#include <array>
#include <vector>

namespace Bunny::Render
{
class VulkanRenderResources;
class MaterialBank;
} // namespace Bunny::Render

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

class BaseWorld
{
  public:
    virtual void update(float deltaTime);
};

class OrganizedWorld : public BaseWorld
{
    struct MeshObjectData
    {
        glm::mat4 mTransform;
        glm::mat4 mInvTransposedTransform;
        Render::IdType mMeshId;
    };

  protected:
    std::vector<MeshObjectData> mSortedMeshObjectData;
};

class World
{
  public:
    constexpr static size_t MAX_OBJECT_COUNT = 10000;

    void update(float deltaTime);

    entt::registry mEntityRegistry;
};

class WorldLoader
{
  public:
    WorldLoader(const Render::VulkanRenderResources* vulkanResources, Render::MaterialBank* materialBank,
        Render::MeshBank<Render::NormalVertex>* meshBank);

    BunnyResult loadGltfToWorld(std::string_view filePath, World& outWorld);
    BunnyResult loadTestWorld(World& outWorld);

  private:
    const Render::VulkanRenderResources* mVulkanResources;
    Render::MaterialBank* mMaterialBank;
    Render::MeshBank<Render::NormalVertex>* mMeshBank;
};

class WorldRenderDataTranslator
{
  public:
    WorldRenderDataTranslator(const Render::VulkanRenderResources* vulkanResources,
        const Render::MeshBank<Render::NormalVertex>* meshBank, const Render::MaterialBank* materialBank);

    BunnyResult initialize();
    BunnyResult translateSceneData(const World* world);
    BunnyResult translateObjectData(const World* world);
    void cleanup();

    const Render::AllocatedBuffer& getSceneBuffer() const { return mSceneDataBuffer; }
    const Render::AllocatedBuffer& getLightBuffer() const { return mLightDataBuffer; }
    const std::vector<Render::RenderBatch>& getRenderBatches() const { return mRenderBatches; }

  private:
    static glm::mat4x4 getEntityGlobalTransform(
        const entt::registry& registry, entt::entity entity, const glm::mat4x4& transformMat);

    const Render::VulkanRenderResources* mVulkanResources;
    const Render::MeshBank<Render::NormalVertex>* mMeshBank;
    const Render::MaterialBank* mMaterialBank;

    Render::AllocatedBuffer mObjectDataBuffer;
    std::vector<Render::ObjectData> mObjectData;
    size_t mInstanceCount;

    Render::AllocatedBuffer mSceneDataBuffer;
    Render::AllocatedBuffer mLightDataBuffer;
    Render::SceneData mSceneData;
    Render::LightData mLightData;

    std::vector<Render::RenderBatch> mRenderBatches;
};

class WorldPhysicsSystem
{
};

class WorldUpdateSystem
{
};
} // namespace Bunny::Engine
