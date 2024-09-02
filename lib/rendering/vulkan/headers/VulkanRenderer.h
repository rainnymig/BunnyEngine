#pragma once

#include <Renderer.h>
#include <QueueFamilyIndices.h>

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
        bool checkValidationLayerSupport(const std::vector<const char*>& validationLayers) const;
        std::vector<const char*> getRequiredExtensions();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;
        void setupDebugMessenger();
        void pickPhysicalDevice();
        void createLogicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice device) const;
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

        class GLFWwindow* mWindow;
        VkInstance mInstance;
        VkDebugUtilsMessengerEXT mDebugMessenger;
        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        VkDevice mDevice;
        VkQueue mGraphicsQueue;
    };
}