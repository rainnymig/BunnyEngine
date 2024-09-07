#pragma once

#include <Renderer.h>
#include <QueueFamilyIndices.h>
#include <SwapChainSupportDetails.h>

#include <vulkan/vulkan.h>

#include <vector>

namespace Bunny::Render
{
class VulkanRenderer : public Renderer
{
  public:
    VulkanRenderer(GLFWwindow* window);

    virtual void initialize() override;
    virtual void render() override;
    virtual void cleanup() override;

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

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);

    class GLFWwindow* mWindow;
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
};
} // namespace Bunny::Render