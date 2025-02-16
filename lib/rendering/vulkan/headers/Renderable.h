#pragma once

#include "Transform.h"

#include "Vertex.h"
#include "BaseVulkanRenderer.h"
#include "Camera.h"
#include "Light.h"
#include "Descriptor.h"

#include <vulkan/vulkan.h>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>
#include <array>

namespace Bunny::Render
{

class Mesh;
class MeshAssetsBank;
class VulkanRenderer;
class Node;
class Scene;

class IRendenrable
{
  public:
    virtual void render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) = 0;
};

class RenderComponent : public IRendenrable
{
  public:
    bool mIsStatic = true;
};

class MeshRenderComponent : public RenderComponent
{
  public:
    explicit MeshRenderComponent(const Mesh* mesh, const Node* owner);

    virtual void render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) override;
    const Mesh* mMesh = nullptr;
    const Node* mOwner = nullptr;
};

class Node : public IRendenrable
{
  public:
    Base::Transform mTransform;
    std::vector<Node*> mChildren;
    Node* mParent;
    Scene* mScene;
    std::unique_ptr<RenderComponent> mRenderComponent;

    virtual void render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) override;
};

class Scene;

class SceneInitializer
{
  public:
    static bool loadFromGltfFile(
        std::string_view filePath, BaseVulkanRenderer* renderer, Scene* scene, MeshAssetsBank* meshAssetsBank);

    static bool makeExampleScene(BaseVulkanRenderer* renderer, Scene* scene, MeshAssetsBank* meshAssetBank);

  private:
    static void addVertex(const glm::vec3& position, const glm::vec3& normal, const glm::vec4& color,
        const glm::vec2& texCoord, std::vector<uint32_t>& indices, std::vector<NormalVertex>& vertices,
        std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap);
    static void addTriangle(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec4& color,
        const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
        std::vector<NormalVertex>& vertices,
        std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap);
    static void addQuad(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& p4,
        const glm::vec4& color, const glm::vec2& texCoordBase, const float scale, std::vector<uint32_t>& indices,
        std::vector<NormalVertex>& vertices,
        std::unordered_map<NormalVertex, uint32_t, NormalVertex::Hash>& vertexToIndexMap);
    static Mesh* createCubeMeshToBank(MeshAssetsBank* bank, BaseVulkanRenderer* renderer);
};

struct SceneData
{
    glm::mat4x4 mViewMatrix;
    glm::mat4x4 mProjMatrix;
    glm::mat4x4 mViewProjMatrix;
};

struct LightData
{
    static constexpr size_t MAX_LIGHT_COUNT = 8;
    glm::vec3 mCameraPos;
    uint32_t mLightCount;
    DirectionalLight mLights[MAX_LIGHT_COUNT];
};

struct ObjectData
{
    glm::mat4 model;
    glm::mat4 invTransModel;
};

class Scene : public IRendenrable
{
  public:
    friend SceneInitializer;
    friend Node;
    friend MeshRenderComponent;

    virtual void render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) override;
    void update(); //  most likely temp, will make proper update later

    void setDevice(VkDevice device) { mDevice = device; }
    void setRenderer(const BaseVulkanRenderer* renderer) { mRenderer = renderer; }
    void destroyScene();
    void buildDescriptorSetLayout();
    void initDescriptorAllocator();
    void initBuffers();
    VkDescriptorSetLayout getSceneDescSetLayout() const { return mSceneDescSetLayout; }
    VkDescriptorSetLayout getObjectDescSetLayout() const { return mObjectDescSetLayout; }

  private:
    void findRootNodes();

    std::unordered_map<size_t, Node> mNodes;
    std::vector<Node*> mRootNodes;
    std::vector<DirectionalLight> mLights;
    Camera mCamera;

    std::array<DescriptorAllocator, MAX_FRAMES_IN_FLIGHT> mDescriptorAllocators;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mSceneDescriptorSets;
    VkDescriptorSetLayout mSceneDescSetLayout;
    VkDescriptorSetLayout mObjectDescSetLayout;
    VkDevice mDevice = nullptr;
    const BaseVulkanRenderer* mRenderer = nullptr;

    //  use separate buffer for now, may merge in the future
    AllocatedBuffer mSceneDataBuffer;
    AllocatedBuffer mLightDataBuffer;
    AllocatedBuffer mObjectDataBuffer;
};

} // namespace Bunny::Render
