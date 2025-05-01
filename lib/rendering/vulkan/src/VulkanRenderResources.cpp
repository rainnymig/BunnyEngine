#include "VulkanRenderResources.h"

#include "Error.h"
#include "ErrorCheck.h"
#include "Window.h"
#include "Helper.h"

#include <VkBootstrap.h>
#include <cassert>

namespace Bunny::Render
{
BunnyResult VulkanRenderResources::initialize(Base::Window* window)
{
    assert(window != nullptr);
    mWindow = window;

    vkb::InstanceBuilder builder;

    //  create VkInstance
    //  make the vulkan instance, with basic debug features
    auto instanceBuildResult = builder
                                   .set_app_name("BunnyEngine")
#ifdef _DEBUG
                                   .request_validation_layers(true)
                                   .enable_validation_layers(true)
                                   .use_default_debug_messenger()
#else
                                   .request_validation_layers(false)
                                   .enable_validation_layers(false)
#endif
                                   .require_api_version(1, 3, 0)
                                   .build();

    if (!instanceBuildResult)
    {
        PRINT_AND_RETURN_VALUE("Failed to create Vulkan instance.", BUNNY_SAD)
    }

    vkb::Instance vkbInstance = instanceBuildResult.value();
    mInstance = vkbInstance.instance;
    mDeletionStack.AddFunction([this]() {
        vkDestroyInstance(mInstance, nullptr);
        mInstance = VK_NULL_HANDLE;
    });

#ifdef _DEBUG
    mDebugMessenger = vkbInstance.debug_messenger;
    mDeletionStack.AddFunction([this]() {
        vkb::destroy_debug_utils_messenger(mInstance, mDebugMessenger);
        mDebugMessenger = VK_NULL_HANDLE;
    });
#endif

    //  creat VkSurface
    if (!BUNNY_SUCCESS(window->createSurface(mInstance, nullptr, &mSurface)))
    {
        PRINT_AND_RETURN_VALUE("failed to create window surface!", BUNNY_SAD)
    }
    mDeletionStack.AddFunction([this]() {
        vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
        mSurface = VK_NULL_HANDLE;
    });

    //  Select VkPhysicalDevice
    //  Note: might review these features in detail later and maybe make them configurable
    VkPhysicalDeviceVulkan13Features features13{};
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12{};
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;
    features12.descriptorBindingPartiallyBound = true;
    features12.descriptorBindingVariableDescriptorCount = true;
    features12.runtimeDescriptorArray = true;
    features12.samplerFilterMinmax = true;

    //  enable usage of std430 uniform buffer
    //  https://docs.vulkan.org/guide/latest/shader_memory_layout.html#VK_KHR_uniform_buffer_standard_layout
    features12.uniformBufferStandardLayout = true;

    VkPhysicalDeviceFeatures featureBasic{};
    featureBasic.samplerAnisotropy = true;
    featureBasic.vertexPipelineStoresAndAtomics = true;

    vkb::PhysicalDeviceSelector selector{vkbInstance};
    auto deviceSelectResult = selector.set_minimum_version(1, 3)
                                  .set_required_features_13(features13)
                                  .set_required_features_12(features12)
                                  .set_required_features(featureBasic)
                                  .set_surface(mSurface)
                                  .select();
    if (!deviceSelectResult)
    {
        PRINT_AND_RETURN_VALUE("Fail to select suitable Vulkan physical devcie.", BUNNY_SAD)
    }
    vkb::PhysicalDevice vkbPhysicalDevice = deviceSelectResult.value();
    mPhysicalDevice = vkbPhysicalDevice.physical_device;

    //  create logical VkDevice
    vkb::DeviceBuilder deviceBuilder{vkbPhysicalDevice};
    auto deviceBuildResult = deviceBuilder.build();
    if (!deviceBuildResult)
    {
        PRINT_AND_RETURN_VALUE("Fail to build Vulkan device.", BUNNY_SAD)
    }
    vkb::Device vkbDevice = deviceBuildResult.value();
    mDevice = vkbDevice.device;
    mDeletionStack.AddFunction([this]() {
        vkDestroyDevice(mDevice, nullptr);
        mDevice = VK_NULL_HANDLE;
    });

    //  get queue family
    if (!BUNNY_SUCCESS(getQueueFromDevice(mGraphicQueue, vkbDevice, vkb::QueueType::graphics)))
    {
        PRINT_AND_RETURN_VALUE("Fail to get graphics queue.", BUNNY_SAD);
    }
    if (!BUNNY_SUCCESS(getQueueFromDevice(mPresentQueue, vkbDevice, vkb::QueueType::present)))
    {
        PRINT_AND_RETURN_VALUE("Fail to get present queue.", BUNNY_SAD);
    }
    if (!BUNNY_SUCCESS(getQueueFromDevice(mComputeQueue, vkbDevice, vkb::QueueType::compute)))
    {
        PRINT_WARNING("Fail to get compute queue, compute pipeline won't work.")
    }
    if (!BUNNY_SUCCESS(getQueueFromDevice(mTransferQueue, vkbDevice, vkb::QueueType::transfer)))
    {
        PRINT_WARNING("Fail to get transfer queue, transfer won't work.")
    }

    //  initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = mPhysicalDevice;
    allocatorInfo.device = mDevice;
    allocatorInfo.instance = mInstance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &mAllocator);

    mDeletionStack.AddFunction([this]() {
        vmaDestroyAllocator(mAllocator);
        mAllocator = nullptr;
    });

    //  create immedate command
    createImmediateCommand();

    return BUNNY_HAPPY;
}

void VulkanRenderResources::cleanup()
{
    mDeletionStack.Flush();
}

BunnyResult VulkanRenderResources::createAndMapBuffer(void* data, VkDeviceSize size, VkBufferUsageFlags bufferUsage,
    VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage, AllocatedBuffer& outBuffer) const
{
    //  create the buffer
    //  add the transfer dst usage to receive data from staging buffer
    outBuffer = createBuffer(size, bufferUsage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, vmaCreateFlags, vmaUsage);

    //  create staging buffer to map and transfer the data
    AllocatedBuffer stagingBuffer = createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY);

    memcpy(stagingBuffer.mAllocationInfo.pMappedData, data, size);

    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(startImmedidateCommand(ImmediateQueueType::Transfer))

    VkBufferCopy bufCopy{0};
    bufCopy.dstOffset = 0;
    bufCopy.srcOffset = 0;
    bufCopy.size = size;

    vkCmdCopyBuffer(mImmediateCommands.at(ImmediateQueueType::Transfer).mBuffer, stagingBuffer.mBuffer,
        outBuffer.mBuffer, 1, &bufCopy);

    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(endAndSubmitImmediateCommand(ImmediateQueueType::Transfer))

    destroyBuffer(stagingBuffer);

    return BUNNY_HAPPY;
}

AllocatedBuffer VulkanRenderResources::createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage,
    VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage) const
{
    AllocatedBuffer newBuffer;

    VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .size = size,
        .usage = bufferUsage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo vmaInfo{
        .flags = vmaCreateFlags,
        .usage = vmaUsage,
    };

    VK_HARD_CHECK(vmaCreateBuffer(
        mAllocator, &bufferInfo, &vmaInfo, &newBuffer.mBuffer, &newBuffer.mAllocation, &newBuffer.mAllocationInfo));

    return newBuffer;
}

AllocatedImage VulkanRenderResources::createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
    VkImageAspectFlags aspectFlags, VkImageLayout layout, uint32_t mipCount) const
{
    AllocatedImage newImage;
    newImage.mFormat = format;
    newImage.mExtent = size;

    VkImageCreateInfo imgCreateInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imgCreateInfo.extent = size;
    imgCreateInfo.mipLevels = mipCount;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.format = format;
    imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imgCreateInfo.initialLayout = layout;
    imgCreateInfo.usage = usage;
    imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imgCreateInfo.pNext = nullptr;

    VmaAllocationCreateInfo allocCreateInfo = {};
    allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
    allocCreateInfo.priority = 1;

    VK_HARD_CHECK(
        vmaCreateImage(mAllocator, &imgCreateInfo, &allocCreateInfo, &newImage.mImage, &newImage.mAllocation, nullptr));

    VkImageViewCreateInfo viewCreateInfo = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewCreateInfo.pNext = nullptr;
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.image = newImage.mImage;
    viewCreateInfo.format = format;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = mipCount;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;

    VK_HARD_CHECK(vkCreateImageView(mDevice, &viewCreateInfo, nullptr, &newImage.mImageView));

    return newImage;
}

void VulkanRenderResources::destroyBuffer(AllocatedBuffer& buffer) const
{
    if (buffer.mBuffer == nullptr)
    {
        return;
    }
    vmaDestroyBuffer(mAllocator, buffer.mBuffer, buffer.mAllocation);
    buffer.mBuffer = nullptr;
    buffer.mAllocation = nullptr;
}

void VulkanRenderResources::destroyImage(AllocatedImage& image) const
{
    vkDestroyImageView(mDevice, image.mImageView, nullptr);
    vmaDestroyImage(mAllocator, image.mImage, image.mAllocation);
    image.mImageView = nullptr;
    image.mImage = nullptr;
    image.mAllocation = nullptr;
}

void VulkanRenderResources::transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format,
    VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const
{
    VkImageMemoryBarrier2 barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    barrier.pNext = nullptr;

    VkImageSubresourceRange range{
        .baseMipLevel = 0,
        .levelCount = mipLevels,
        .baseArrayLayer = 0,
        .layerCount = 1,
    };
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange = range;
    barrier.image = image;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    barrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &depInfo);
}

BunnyResult VulkanRenderResources::immediateTransitionImageLayout(
    VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) const
{
    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(startImmedidateCommand())

    this->transitionImageLayout(
        mImmediateCommands.at(ImmediateQueueType::Graphics).mBuffer, image, format, oldLayout, newLayout, mipLevels);

    BUNNY_CHECK_SUCCESS_OR_RETURN_RESULT(endAndSubmitImmediateCommand())

    return BUNNY_HAPPY;
}

VkFormat VulkanRenderResources::findSupportedFormat(
    std::span<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

VulkanRenderResources::~VulkanRenderResources()
{
    cleanup();
}

BunnyResult VulkanRenderResources::getQueueFromDevice(
    Queue& queue, const vkb::Device& device, vkb::QueueType queueType) const
{
    auto queueResult = device.get_queue(queueType);
    auto idxResult = device.get_queue_index(queueType);
    if (queueResult && idxResult)
    {
        queue.mQueue = queueResult.value();
        queue.mQueueFamilyIndex = idxResult.value();
        return BUNNY_HAPPY;
    }
    return BUNNY_SAD;
}

BunnyResult VulkanRenderResources::createImmediateCommand()
{
    assert(mGraphicQueue.mQueueFamilyIndex.has_value());

    ImmediateCommand& graphicsCommand = mImmediateCommands[ImmediateQueueType::Graphics];
    ImmediateCommand& transferCommand = mImmediateCommands[ImmediateQueueType::Transfer];

    //  create command pool for graphics queue
    VkCommandPoolCreateInfo poolInfo = makeCommandPoolCreateInfo(
        mGraphicQueue.mQueueFamilyIndex.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &graphicsCommand.mPool));

    mDeletionStack.AddFunction([this]() {
        vkDestroyCommandPool(mDevice, mImmediateCommands[ImmediateQueueType::Graphics].mPool, nullptr);
        mImmediateCommands[ImmediateQueueType::Graphics].mPool = nullptr;
    });

    //  create command buffer for graphics queue
    VkCommandBufferAllocateInfo allocInfo = makeCommandBufferAllocateInfo(graphicsCommand.mPool, 1);
    VK_CHECK_OR_RETURN_BUNNY_SAD(vkAllocateCommandBuffers(mDevice, &allocInfo, &graphicsCommand.mBuffer));

    //  if transfer queue is not the same as graphics queue then create commands for transfer
    //  otherwise use the same commands
    if (mTransferQueue.mQueueFamilyIndex.has_value() &&
        mTransferQueue.mQueueFamilyIndex.value() != mGraphicQueue.mQueueFamilyIndex.value())
    {
        //  create command pool for transfer queue
        VkCommandPoolCreateInfo poolInfo = makeCommandPoolCreateInfo(
            mTransferQueue.mQueueFamilyIndex.value(), VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateCommandPool(mDevice, &poolInfo, nullptr, &transferCommand.mPool));

        mDeletionStack.AddFunction([this]() {
            vkDestroyCommandPool(mDevice, mImmediateCommands[ImmediateQueueType::Transfer].mPool, nullptr);
            mImmediateCommands[ImmediateQueueType::Transfer].mPool = nullptr;
        });

        //  create command buffer for transfer queue
        VkCommandBufferAllocateInfo allocInfo = makeCommandBufferAllocateInfo(transferCommand.mPool, 1);
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkAllocateCommandBuffers(mDevice, &allocInfo, &transferCommand.mBuffer));
    }
    else
    {
        transferCommand.mPool = graphicsCommand.mPool;
        transferCommand.mBuffer = graphicsCommand.mBuffer;
    }

    //  create immediate fence for synchronizing immediate command submits
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkCreateFence(mDevice, &fenceInfo, nullptr, &mImmediateFence));

    mDeletionStack.AddFunction([this]() { vkDestroyFence(mDevice, mImmediateFence, nullptr); });

    return BUNNY_HAPPY;
}

BunnyResult VulkanRenderResources::startImmedidateCommand(ImmediateQueueType cmdType) const
{
    VK_CHECK_OR_RETURN_BUNNY_SAD(vkResetFences(mDevice, 1, &mImmediateFence));

    VkCommandBuffer cmdBuf = mImmediateCommands.at(cmdType).mBuffer;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkResetCommandBuffer(cmdBuf, 0));

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkBeginCommandBuffer(cmdBuf, &beginInfo));

    return BUNNY_HAPPY;
}

BunnyResult VulkanRenderResources::endAndSubmitImmediateCommand(ImmediateQueueType cmdType) const
{
    VkCommandBuffer cmdBuf = mImmediateCommands.at(cmdType).mBuffer;

    VK_CHECK_OR_RETURN_BUNNY_SAD(vkEndCommandBuffer(cmdBuf));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuf;

    if (cmdType == ImmediateQueueType::Graphics)
    {
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkQueueSubmit(mGraphicQueue.mQueue, 1, &submitInfo, mImmediateFence));
    }
    else if (cmdType == ImmediateQueueType::Transfer)
    {
        VK_CHECK_OR_RETURN_BUNNY_SAD(vkQueueSubmit(mTransferQueue.mQueue, 1, &submitInfo, mImmediateFence));
    }
    VK_CHECK_OR_RETURN_BUNNY_SAD(vkWaitForFences(mDevice, 1, &mImmediateFence, true, 9999999999));

    return BUNNY_HAPPY;
}

} // namespace Bunny::Render
