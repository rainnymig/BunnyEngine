#include "VulkanRenderResources.h"

#include "Error.h"

#include <VkBootstrap.h>

namespace Bunny::Render
{
BunnyResult VulkanRenderResources::initialize(Base::Window* window)
{
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

    VkPhysicalDeviceFeatures featureBasic{};
    featureBasic.samplerAnisotropy = true;

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

    return BUNNY_HAPPY;
}

void VulkanRenderResources::cleanup()
{
    mDeletionStack.Flush();
}

VulkanRenderResources::~VulkanRenderResources()
{
    cleanup();
}

BunnyResult VulkanRenderResources::getQueueFromDevice(Queue& queue, const vkb::Device& device, vkb::QueueType queueType)
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

} // namespace Bunny::Render
