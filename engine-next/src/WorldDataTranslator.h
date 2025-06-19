#pragma once

#include "BunnyResult.h"
#include "Fundamentals.h"
#include "MeshBank.h"
#include "Vertex.h"
#include "ShaderData.h"
#include "World.h"

#include <unordered_map>

namespace Bunny::Render
{
class VulkanRenderResources;
} // namespace Bunny::Render

namespace Bunny::Engine
{
class WorldRenderDataTranslator
{
  public:
    WorldRenderDataTranslator(
        const Render::VulkanRenderResources* vulkanResources, const Render::MeshBank<Render::NormalVertex>* meshBank);

    BunnyResult initialize();
    BunnyResult updateSceneData(const World* world);
    BunnyResult updatePbrWorldData(const World* world); //  update camera and light data
    BunnyResult updateObjectData(const World* world);
    BunnyResult initObjectDataBuffer(const World* world);
    void cleanup();

    const Render::AllocatedBuffer& getSceneBuffer() const { return mSceneDataBuffer; }
    const Render::AllocatedBuffer& getLightBuffer() const { return mLightDataBuffer; }
    const Render::AllocatedBuffer& getObjectBuffer() const { return mObjectDataBuffer; }
    const Render::AllocatedBuffer& getPbrCameraBuffer() const { return mPbrCameraBuffer; }
    const Render::AllocatedBuffer& getPbrLightBuffer() const { return mPbrLightBuffer; }
    const size_t getObjectBufferSize() const { return mObjectData.size() * sizeof(Render::ObjectData); }
    const uint32_t getObjectCount() const { return mObjectData.size(); }

    const std::unordered_map<Render::IdType, size_t>& getMeshInstanceCounts() const { return mMeshInstanceCounts; }

  private:
    static void getEntityGlobalTransform(const entt::registry& registry, entt::entity entity,
        const glm::mat4x4& transform, const glm::vec3& scale, glm::mat4x4& outTransform, glm::vec3& outScale);

    const Render::VulkanRenderResources* mVulkanResources;
    const Render::MeshBank<Render::NormalVertex>* mMeshBank;

    Render::AllocatedBuffer mObjectDataBuffer;
    std::vector<Render::ObjectData> mObjectData;

    std::unordered_map<Render::IdType, size_t> mMeshInstanceCounts;

    Render::AllocatedBuffer mSceneDataBuffer;
    Render::AllocatedBuffer mLightDataBuffer;
    Render::SceneData mSceneData;
    Render::LightData mLightData;

    //  PBR
    Render::AllocatedBuffer mPbrCameraBuffer;
    Render::AllocatedBuffer mPbrLightBuffer;
    Render::PbrCameraData mPbrCameraData;
    Render::PbrLightData mPbrLightData;
};
} // namespace Bunny::Engine
