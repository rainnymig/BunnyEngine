#pragma once

#include "FunctionStack.h"
#include "Window.h"
#include "BunnyResult.h"

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <VkBootstrap.h>

#include <optional>

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

    const Queue& getGraphicQueue() const { return mGraphicQueue; }
    const Queue& getPresentQueue() const { return mPresentQueue; }
    const Queue& getComputeQueue() const { return mComputeQueue; }
    const Queue& getTransferQueue() const { return mTransferQueue; }

    ~VulkanRenderResources();

  private:
    BunnyResult getQueueFromDevice(Queue& queue, const vkb::Device& device, vkb::QueueType queueType);

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

    VmaAllocator mAllocator = nullptr;

    //  deletion stack
    Base::FunctionStack<> mDeletionStack;
};
} // namespace Bunny::Render
