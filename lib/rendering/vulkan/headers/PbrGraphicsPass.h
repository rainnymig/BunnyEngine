#pragma once

#include "Fundamentals.h"
#include "BunnyResult.h"
#include "FunctionStack.h"
#include "MeshBank.h"
#include "Vertex.h"

#include <vulkan/vulkan.h>

#include <string_view>

namespace Bunny::Render
{
class VulkanRenderResources;
class VulkanGraphicsRenderer;
class PbrMaterialBank;

class PbrGraphicsPass
{
  public:
    PbrGraphicsPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank, std::string_view vertShader,
        std::string_view fragShader);
    virtual ~PbrGraphicsPass();

    BunnyResult initializePass();
    virtual void draw() const = 0;
    virtual void cleanup();

  protected:
    virtual BunnyResult initPipeline();
    virtual BunnyResult initDescriptors();

    const VulkanRenderResources* mVulkanResources;
    const VulkanGraphicsRenderer* mRenderer;
    const PbrMaterialBank* mMaterialBank;
    const MeshBank<NormalVertex>* mMeshBank;

    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;
    Base::FunctionStack<> mDeletionStack;
    bool mIsInitialized = false;
};
} // namespace Bunny::Render
