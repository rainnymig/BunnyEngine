#pragma once

#include "Transform.h"

#include <vulkan/vulkan.h>

namespace Bunny::Render
{
class IRendenrable
{
  public:
    virtual void render(VkCommandBuffer commandBuffer) = 0;
};

class Scene : public IRendenrable
{
  public:
    virtual void render(VkCommandBuffer commandBuffer) override;
};

class GameObject
{
};

class RenderComponent : public IRendenrable
{
  public:
    Base::Transform mTransform;
    bool mIsStatic = true;
};

class Mesh;

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

} // namespace Bunny::Render
