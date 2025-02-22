#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <string>

namespace Bunny::Base
{
class Window
{
  public:
    bool initialize(int width, int height, bool isFullscreen = false, const std::string& name = "");

    //  create surface for Vulkan
    //  can have other overload in the future (e.g. for DirectX 12)
    const VkResult createSurface(
        VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) const;

    void getFrameBufferSize(int& width, int& height) const;

    template <typename CallbackT>
    void registerFrameBufferResizeCallback(CallbackT&& callback)
    {
        glfwSetFramebufferSizeCallback(
            mWindow, [](GLFWwindow* window, int width, int height) { callback(width, height); });
    }

    void destroyAndTerminate();

    ~Window();

  private:
    GLFWwindow* mWindow = nullptr;
};
} // namespace Bunny::Base