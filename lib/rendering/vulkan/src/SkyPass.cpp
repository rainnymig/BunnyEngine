#include "SkyPass.h"

#include "Error.h"

namespace Bunny::Render
{
SkyPass::SkyPass(const VulkanRenderResources* vulkanResources, const VulkanGraphicsRenderer* renderer,
    std::string_view cloudShaderPath)
    : super(vulkanResources, renderer, nullptr, nullptr),
      mCloudShaderPath(cloudShaderPath)
{
}

void SkyPass::draw() const
{
}

BunnyResult SkyPass::initPipeline()
{
    return BUNNY_HAPPY;
}

BunnyResult SkyPass::initDescriptors()
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(initDescriptorLayouts());

    VkDevice device = mVulkanResources->getDevice();

    DescriptorAllocator::PoolSize poolSizes[] = {
        {.mType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .mRatio = 3},
        {.mType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .mRatio = 2},
        {.mType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          .mRatio = 2},
    };
    mDescriptorAllocator.init(device, 2, poolSizes);

    VkDescriptorSetLayout descLayouts[] = {mCloudDescSetLayout, mTextureDescSetLayout};
    for (FrameData& frame : mFrameData)
    {
        mDescriptorAllocator.allocate(device, descLayouts, &frame.mCloudDescSet, 2);
    }

    mDeletionStack.AddFunction([this]() { mDescriptorAllocator.destroyPools(mVulkanResources->getDevice()); });

    return BUNNY_HAPPY;
}

BunnyResult SkyPass::initDataAndResources()
{
    return BUNNY_HAPPY;
}

BunnyResult SkyPass::initDescriptorLayouts()
{
    VkDescriptorSetLayoutBinding descBinding{
        0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};

    VkDevice device = mVulkanResources->getDevice();

    DescriptorLayoutBuilder builder;
    //  light data
    builder.addBinding(descBinding);
    //  cloud data
    descBinding.binding = 1;
    builder.addBinding(descBinding);
    //  render data
    descBinding.binding = 2;
    builder.addBinding(descBinding);
    mCloudDescSetLayout = builder.build(device);

    builder.clear();
    //  3d textures
    descBinding.binding = 0;
    descBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descBinding.descriptorCount = 2; //  main noise, detail noise
    builder.addBinding(descBinding);
    //  2d textures
    descBinding.binding = 1;
    descBinding.descriptorCount = 4; // blue noise, weather map, depth image, previous cloud texture
    builder.addBinding(descBinding);
    //  output cloud texture
    descBinding.binding = 2;
    descBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    descBinding.descriptorCount = 1;
    builder.addBinding(descBinding);
    //  output fog shadow texture
    descBinding.binding = 3;
    builder.addBinding(descBinding);
    mTextureDescSetLayout = builder.build(device);

    mDeletionStack.AddFunction([this, device]() {
        vkDestroyDescriptorSetLayout(device, mCloudDescSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, mTextureDescSetLayout, nullptr);
    });

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
