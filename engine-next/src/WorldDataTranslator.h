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
class MaterialBank;
} // namespace Bunny::Render

namespace Bunny::Engine
{
class WorldRenderDataTranslator
{
  public:
    WorldRenderDataTranslator(const Render::VulkanRenderResources* vulkanResources,
        const Render::MeshBank<Render::NormalVertex>* meshBank, const Render::MaterialBank* materialBank);

    BunnyResult initialize();
    BunnyResult updateSceneData(const World* world);
    BunnyResult updateObjectData(const World* world);
    BunnyResult initObjectDataBuffer(const World* world);
    void cleanup();

    const Render::AllocatedBuffer& getSceneBuffer() const { return mSceneDataBuffer; }
    const Render::AllocatedBuffer& getLightBuffer() const { return mLightDataBuffer; }
    const Render::AllocatedBuffer& getObjectBuffer() const { return mObjectDataBuffer; }
    const size_t getObjectBufferSize() const { return mObjectData.size() * sizeof(Render::ObjectData); }

    const std::unordered_map<Render::IdType, size_t>& getMeshInstanceCounts() const { return mMeshInstanceCounts; }

  private:
    static glm::mat4x4 getEntityGlobalTransform(
        const entt::registry& registry, entt::entity entity, const glm::mat4x4& transformMat);

    const Render::VulkanRenderResources* mVulkanResources;
    const Render::MeshBank<Render::NormalVertex>* mMeshBank;
    const Render::MaterialBank* mMaterialBank;

    Render::AllocatedBuffer mObjectDataBuffer;
    std::vector<Render::ObjectData> mObjectData;

    std::unordered_map<Render::IdType, size_t> mMeshInstanceCounts;

    Render::AllocatedBuffer mSceneDataBuffer;
    Render::AllocatedBuffer mLightDataBuffer;
    Render::SceneData mSceneData;
    Render::LightData mLightData;
};
} // namespace Bunny::Engine
