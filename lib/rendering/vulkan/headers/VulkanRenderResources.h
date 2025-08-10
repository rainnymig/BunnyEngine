#pragma once

#include "FunctionStack.h"
#include "BunnyResult.h"
#include "Fundamentals.h"

#include <volk.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include <optional>
#include <span>
#include <functional>
#include <map>

//  uncomment this line if want to use Nvidia Nsight Graphics to debug
// #define USE_NSIGHT_DEBUG

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

    enum class CommandQueueType
    {
        Graphics,
        Transfer
    };

    struct ImmediateCommand
    {
        VkCommandPool mPool;
        VkCommandBuffer mBuffer;
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

    BunnyResult createBufferWithData(const void* data, VkDeviceSize size, VkBufferUsageFlags bufferUsage,
        VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage, AllocatedBuffer& outBuffer,
        VkDeviceSize minAlignment = 0) const;
    BunnyResult createImageWithData(void* data, VkDeviceSize dataSize, VkExtent3D imageExtent, VkFormat format,
        VkImageUsageFlags usage, VkImageAspectFlags aspectFlags, VkImageLayout layout, AllocatedImage& outImage) const;

    AllocatedBuffer createBuffer(VkDeviceSize size, VkBufferUsageFlags bufferUsage,
        VmaAllocationCreateFlags vmaCreateFlags, VmaMemoryUsage vmaUsage, VkDeviceSize minAlignment = 0) const;
    AllocatedImage createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
        VkImageAspectFlags aspectFlags, VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED, uint32_t mipCount = 1) const;
    void destroyBuffer(AllocatedBuffer& buffer) const;
    void destroyImage(AllocatedImage& image) const;
    void copyBuffer(VkCommandBuffer cmd, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    void transitionBufferAccess(VkCommandBuffer cmd, VkBuffer buffer, VkAccessFlags srcAccessMask,
        VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage) const;

    void transitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout,
        VkImageLayout newLayout, uint32_t mipLevels = 1) const;
    BunnyResult immediateTransitionImageLayout(
        VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1) const;

    VkDeviceAddress getBufferDeviceAddress(const AllocatedBuffer& buffer) const;

    VkFormat findSupportedFormat(
        std::span<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    VkCommandBuffer startImmedidateCommand(CommandQueueType cmdType = CommandQueueType::Graphics) const;
    BunnyResult endAndSubmitImmediateCommand(CommandQueueType cmdType = CommandQueueType::Graphics) const;

    //  query physical device properties
    //  should pass in a pointer to physical device property struct
    //  https://registry.khronos.org/vulkan/specs/latest/man/html/VkPhysicalDeviceProperties2.html#VUID-VkPhysicalDeviceProperties2-pNext-pNext
    void getPhysicalDeviceProperties(void* properties) const;

    ~VulkanRenderResources();

  private:
    BunnyResult getQueueFromDevice(Queue& queue, const vkb::Device& device, vkb::QueueType queueType) const;
    BunnyResult createImmediateCommand();

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
    std::map<CommandQueueType, ImmediateCommand> mImmediateCommands;
    VkFence mImmediateFence;

    VmaAllocator mAllocator = nullptr;

    //  deletion stack
    Base::FunctionStack<> mDeletionStack;
};
} // namespace Bunny::Render
