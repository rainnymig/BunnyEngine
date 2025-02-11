#pragma once

#include "Transform.h"

#include "Vertex.h"
#include "BaseVulkanRenderer.h"

#include <vulkan/vulkan.h>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <memory>

namespace Bunny::Render
{

class Mesh;
class MeshAssetsBank;
class VulkanRenderer;
class Node;

class IRendenrable
{
  public:
    virtual void render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) const = 0;
};

class RenderComponent : public IRendenrable
{
  public:
    bool mIsStatic = true;
};

class MeshRenderComponent : public RenderComponent
{
  public:
    struct RenderPushConstant
    {
        glm::mat4 model;
        glm::mat4 invTransModel;
    };

    explicit MeshRenderComponent(const Mesh* mesh, const Node* owner);

    virtual void render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) const override;
    const Mesh* mMesh = nullptr;
    const Node* mOwner = nullptr;
};

class Node : public IRendenrable
{
  public:
    Base::Transform mTransform;
    std::vector<Node*> mChildren;
    Node* mParent;
    std::unique_ptr<RenderComponent> mRenderComponent;

    virtual void render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) const override;
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

class Scene : public IRendenrable
{
  public:
    friend SceneInitializer;
    virtual void render(VkCommandBuffer commandBuffer, const glm::mat4& parentTransformMatrix) const override;

  private:
    void findRootNodes();

    std::unordered_map<size_t, Node> mNodes;
    std::vector<const Node*> mRootNodes;
};

} // namespace Bunny::Render
