#pragma once

#include "PbrGraphicsPass.h"
#include "Descriptor.h"

#include <array>

namespace Bunny::Render
{

class VulkanRenderResources;
class VulkanGraphicsRenderer;
class PbrMaterialBank;

class PbrForwardPass : public PbrGraphicsPass
{
  public:
    PbrForwardPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
        const PbrMaterialBank* materialBank, const MeshBank<NormalVertex>* meshBank, std::string_view vertShader,
        std::string_view fragShader);

    virtual void draw() const override;

    void buildDrawCommands();
    void updateDrawInstanceCounts(std::unordered_map<IdType, size_t> meshInstanceCounts);
    void prepareDrawCommandsForFrame();
    void linkWorldData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData);
    void linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);
    void linkShadowData(std::array<VkImageView, MAX_FRAMES_IN_FLIGHT> shadowImageViews);

    const AllocatedBuffer& getDrawCommandBuffer() const { return mDrawCommandsBuffer; }
    const size_t getDrawCommandBufferSize() const;
    const AllocatedBuffer& getInstanceObjectBuffer() const { return mInstanceObjectBuffer; }
    const size_t getInstanceObjectBufferSize() const { return mInstanceObjectBufferSize; }

  protected:
    struct FrameData
    {
        VkDescriptorSet mWorldDescSet;
        VkDescriptorSet mObjectDescSet;
        VkDescriptorSet mMaterialDescSet;
        VkDescriptorSet mEffectDescSet; // for shadows
    };

    virtual BunnyResult initPipeline() override;
    virtual BunnyResult initDescriptors() override;

  private:
    using super = PbrGraphicsPass;

    std::string_view mVertexShaderPath;
    std::string_view mFragmentShaderPath;

    AllocatedBuffer mDrawCommandsBuffer;
    AllocatedBuffer mInitialDrawCommandBuffer;
    std::vector<VkDrawIndexedIndirectCommand> mDrawCommandsData;
    AllocatedBuffer mInstanceObjectBuffer;
    size_t mInstanceObjectBufferSize;

    DescriptorAllocator mDescriptorAllocator;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
};
} // namespace Bunny::Render
