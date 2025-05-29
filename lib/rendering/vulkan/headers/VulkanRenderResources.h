#pragma once

#include "FunctionStack.h"
#include "BunnyResult.h"
#include "Fundamentals.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include <optional>
#include <span>
#include <functional>
#include <map>

namespace Bunny::Base
{
class Window;
}

namespace Bunny::Render
{
class VulkanRenderResources
{
  public:
    struct Queue
    {
        VkQueue mQueue = nullptr;
        std::optional<uint32_t> mQueueFamilyIndex;
    };

    BunnyResult initialize(Base::Window* window);
    void cleanup();

    Base::Window* getWindow() const { return mWindow; }
    VkInstance getInstance() const { return mInstance; }
    VkSurfaceKHR getSurface() const { return mSurface; }
    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
    VkDevice getDevice() const { return mDevice; }
    const Queue& getGraphicQueue() const { return mGraphicQueue; }
    const Queue& getPresentQueue() const { return mPresentQueue; }
    const Queue& getComputeQueue() const { return mComputeQueue; }
    const Queue& getTransferQueue() const { return mTransferQueue; }

    BunnyResult createBufferWithData(void* data, VkDeviceSize size, VkBufferUsageFlags bufferUsage,
        VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage, AllocatedBuffer& outBuffer) const;
    BunnyResult createImageWithData(void* data, VkDeviceSize dataSize, VkExtent3D imageExtent, VkFormat format,
        VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, VkImageLayout layout, AllocatedImage& outImage) const;

    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage,
        VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage) const;
    AllocatedImage createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
        VkImageAspectFlags aspectFlags, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED, uint32_t mipCount = 1) const;
    void destroyBuffer(AllocatedBuffer& buffer) const;
    void destroyImage(AllocatedImage& image) const;

    void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout,
        VkImageLayout newLayout, uint32_t mipLevels = 1) const;
    BunnyResult immediateTransitionImageLayout(
        VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1) const;

    VkFormat findSupportedFormat(
        std::span<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    ~VulkanRenderResources();

  private:
    enum class ImmediateQueueType
    {
        Graphics,
        Transfer
    };

    struct ImmediateCommand
    {
        VkCommandPool mPool;
        VkCommandBuffer mBuffer;
    };

    BunnyResult getQueueFromDevice(Queue& queue, const vkb::Device& device, vkb::QueueType queueType) const;
    BunnyResult createImmediateCommand();
    BunnyResult startImmedidateCommand(ImmediateQueueType cmdType = ImmediateQueueType::Graphics) const;
    BunnyResult endAndSubmitImmediateCommand(ImmediateQueueType cmdType = ImmediateQueueType::Graphics) const;

    Base::Window* mWindow = nullptr;
    VkInstance mInstance = VK_NULL_HANDLE;
#ifdef _DEBUG
    VkDebugUtilsMessengerEXT mDebugMessenger = VK_NULL_HANDLE;
#endif
    VkSurfaceKHR mSurface = VK_NULL_HANDLE;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice = VK_NULL_HANDLE;

    Queue mGraphicQueue;
    Queue mPresentQueue;
    Queue mComputeQueue;
    Queue mTransferQueue;

    //  command pool for submitting immediate commands like image format transition or copy buffer
    std::map<ImmediateQueueType, ImmediateCommand> mImmediateCommands;
    VkFence mImmediateFence;

    VmaAllocator mAllocator = nullptr;

    //  deletion stack
    Base::FunctionStack<> mDeletionStack;
};
} // namespace Bunny::Render
