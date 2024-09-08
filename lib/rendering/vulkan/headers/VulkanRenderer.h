#pragma once

#include <Renderer.h>
#include <QueueFamilyIndices.h>
#include <SwapChainSupportDetails.h>

#include <vulkan/vulkan.h>

#include <vector>
#include <array>

namespace Bunny::Render
{

constexpr size_t MAX_FRAMES_IN_FLIGHT = 2;

class VulkanRenderer : public Renderer
{
  public:
    VulkanRenderer(GLFWwindow* window);

    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanUp() override;

    void setFrameBufferResized();

  private:
    void createInstance();
    void setupDebugMessenger();
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void createImageViews();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFrameBuffers();
    void createCommandPool();
    void createCommandBuffer();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void createSyncObjects();
    void recreateSwapChain();
    void createVertexBuffer();

    void cleanUpSwapChain();

    bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) const;
    std::vector<const char*> getRequiredExtensions() const;
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;
    bool isDeviceSuitable(VkPhysicalDevice device) const;
    bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) const;
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const;
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) const;
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
    VkShaderModule createShaderModule(const std::vector<std::byte>& code) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    //  window
    class GLFWwindow* mWindow;

    //  vulkan rendering objects
    VkInstance mInstance;
    VkDebugUtilsMessengerEXT mDebugMessenger;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkDevice mDevice;
    QueueFamilyIndices mQueueFamilyIndices;
    VkQueue mGraphicsQueue;
    VkQueue mPresentQueue;
    VkSurfaceKHR mSurface;
    VkSwapchainKHR mSwapChain;
    std::vector<VkImage> mSwapChainImages;
    std::vector<VkImageView> mSwapChainImageViews;
    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;
    VkRenderPass mRenderPass;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    VkCommandPool mCommandPool;
    std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> mCommandBuffers;

    //  rendering buffers
    VkBuffer mVertexBuffer;
    VkDeviceMemory mVertexBufferMemory;

    //  vulkan sync objects
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mImageAvailableSemaphores;
    std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> mRenderFinishedSemaphores;
    std::array<VkFence, MAX_FRAMES_IN_FLIGHT> mInFlightFences;

    //  multi frame inflight objects
    uint32_t mCurrentFrameId = 0;

    //  explicitly handle window (framebuffer) resize
    bool mFrameBufferResized = false;
};
} // namespace Bunny::Render