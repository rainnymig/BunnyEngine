#pragma once

#include "BunnyResult.h"

#include <volk.h>
#include <GLFW/glfw3.h>
#include <string>

namespace Bunny::Base
{
class Window
{
  public:
    BunnyResult initialize(int width, int height, bool isFullscreen = false, const std::string& name = "");

    //  create surface for Vulkan
    //  can have other overload in the future (e.g. for DirectX 12)
    BunnyResult createSurface(VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) const;
    void getFrameBufferSize(int& outWidth, int& outHeight) const;
    //  get frame buffer size from window, but would only return when window is not minimized, otherwise wait
    void getFrameBufferSizeNotMinimized(int& outWidth, int& outHeight) const;
    void destroyAndTerminate();

    template <typename CallbackT>
    void registerFrameBufferResizeCallback(CallbackT&& callback)
    {
        glfwSetFramebufferSizeCallback(
            mGlfwWindow, [](GLFWwindow* window, int width, int height) { callback(width, height); });
    }

    //  return true when window should close
    bool processWindowEvent();

    GLFWwindow* getRawGlfwWindow() const { return mGlfwWindow; }

    ~Window();

  private:
    GLFWwindow* mGlfwWindow = nullptr;
};
} // namespace Bunny::Base