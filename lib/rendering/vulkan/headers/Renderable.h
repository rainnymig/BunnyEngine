#pragma once

#include "Transform.h"

#include <vulkan/vulkan.h>
#include <string_view>
#include <vector>
#include <unordered_map>

namespace Bunny::Render
{

class Mesh;
class MeshAssetsBank;

class IRendenrable
{
  public:
    virtual void render(VkCommandBuffer commandBuffer) = 0;
};

class RenderComponent : public IRendenrable
{
  public:
    Base::Transform mTransform;
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

    explicit MeshRenderComponent(const Mesh* mesh);

    virtual void render(VkCommandBuffer commandBuffer) override;
    const Mesh* mMesh = nullptr;
};

class Node
{
    public:
    Base::Transform mTransform;
    std::vector<Node*> mChildren;
    Node* mParent;
    RenderComponent* mRenderer;
};

class Scene : public IRendenrable
{
  public:
    explicit Scene(MeshAssetsBank* bank);

    bool loadFromGltfFile(std::string_view filePath);

    virtual void render(VkCommandBuffer commandBuffer) override;

  private:
    MeshAssetsBank* mMeshAssetsBank;
    std::unordered_map<size_t, Node> mNodes;
    std::vector<Node*> mRootNodes;
};

} // namespace Bunny::Render
