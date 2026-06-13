#pragma once

#include "BunnyResult.h"
#include "Fundamentals.h"
#include "MeshBank.h"
#include "Vertex.h"
#include "ShaderData.h"
#include "World.h"

#include <unordered_map>

namespace Bunny
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;
} // namespace Bunny

namespace Bunny
{
class WorldRenderDataTranslator
{
  public:
    WorldRenderDataTranslator(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const MeshBank<NormalVertex>* meshBank);

    BunnyResult initialize();
    BunnyResult updatePbrWorldData(const World* world); //  update camera and light data
    BunnyResult updateObjectData(const World* world);
    BunnyResult initObjectDataBuffer(const World* world);
    void cleanup();

    const AllocatedBuffer& getObjectBuffer() const { return mObjectDataBuffer; }
    const AllocatedBuffer& getPbrCameraBuffer() const { return mPbrCameraBuffer; }
    const AllocatedBuffer& getPbrLightBuffer() const { return mPbrLightBuffer; }
    const size_t getObjectBufferSize() const { return mObjectData.size() * sizeof(ObjectData); }
    const uint32_t getObjectCount() const { return mObjectData.size(); }
    const std::vector<ObjectData>& getObjectData() const { return mObjectData; }

    const std::unordered_map<IdType, size_t>& getMeshInstanceCounts() const { return mMeshInstanceCounts; }

    void showImguiControlPanel(World* world);

  private:
    static void getEntityGlobalTransform(const entt::registry& registry, entt::entity entity,
        const glm::mat4x4& transform, const glm::vec3& scale, glm::mat4x4& outTransform, glm::vec3& outScale);

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
    const MeshBank<NormalVertex>* mMeshBank;

    AllocatedBuffer mObjectDataBuffer;
    std::vector<ObjectData> mObjectData;

    std::unordered_map<IdType, size_t> mMeshInstanceCounts;

    //  PBR
    AllocatedBuffer mPbrCameraBuffer;
    AllocatedBuffer mPbrLightBuffer;
    PbrCameraData mPbrCameraData;
    PbrLightData mPbrLightData;
};
} // namespace Bunny
