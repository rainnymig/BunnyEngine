#pragma once

#include "PbrGraphicsPass.h"
#include "Descriptor.h"

#include <array>

namespace Bunny::Render
{
class PbrForwardPass : public PbrGraphicsPass
{
  public:
    virtual void draw() const override;

    void buildDrawCommands();
    void updateDrawInstanceCounts(std::unordered_map<IdType, size_t> meshInstanceCounts);
    void prepareDrawCommandsForFrame();
    void linkSceneData(const AllocatedBuffer& lightData, const AllocatedBuffer& cameraData);
    void linkObjectData(const AllocatedBuffer& objectBuffer, size_t bufferSize);

  protected:
    struct FrameData
    {
        VkDescriptorSet mSceneDescSet;
        VkDescriptorSet mObjectDescSet;
        VkDescriptorSet mMaterialDescSet;
    };

    virtual BunnyResult initPipeline() override;
    virtual BunnyResult initDescriptors() override;

    AllocatedBuffer mDrawCommandsBuffer;
    AllocatedBuffer mInitialDrawCommandBuffer;
    std::vector<VkDrawIndexedIndirectCommand> mDrawCommandsData;
    AllocatedBuffer mInstanceObjectBuffer;
    size_t mInstanceObjectBufferSize;

    DescriptorAllocator mDescriptorAllocator;
    std::array<FrameData, MAX_FRAMES_IN_FLIGHT> mFrameData;
};
} // namespace Bunny::Render
