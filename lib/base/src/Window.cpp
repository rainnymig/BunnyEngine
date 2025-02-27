#include "Window.h"

#include "Error.h"

namespace Bunny::Base
{
BunnyResult Window::initialize(int width, int height, bool isFullscreen, const std::string& name)
{
    //  Initialize the library
    if (!glfwInit())
    {
        return BUNNY_SAD;
    }

    //  Vulkan specific, see https://www.glfw.org/docs/3.3/vulkan_guide.html
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, isFullscreen ? GLFW_FALSE : GLFW_TRUE);

    mGlfwWindow = glfwCreateWindow(width, height, name.c_str(), isFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!mGlfwWindow)
    {
        glfwTerminate();
        PRINT_AND_RETURN_VALUE("Failed to create glfw window.", BUNNY_SAD)
    }

    //  Make the window's context current
    glfwMakeContextCurrent(mGlfwWindow);

    return BUNNY_HAPPY;
}

BunnyResult Window::createSurface(
    VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) const
{
    VkResult result = glfwCreateWindowSurface(instance, mGlfwWindow, allocator, surface);

    return result == VK_SUCCESS ? BUNNY_HAPPY : BUNNY_SAD;
}

void Window::getFrameBufferSize(int& outWidth, int& outHeight) const
{
    glfwGetFramebufferSize(mGlfwWindow, &outWidth, &outHeight);
}

void Window::getFrameBufferSizeNotMinimized(int& outWidth, int& outHeight) const
{
    //  handle window minimization
    //  if window is minimized, pause and wait
    int width = 0, height = 0;
    glfwGetFramebufferSize(mGlfwWindow, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(mGlfwWindow, &width, &height);
        glfwWaitEvents();
    }

    outWidth = width;
    outHeight = height;
}

void Window::destroyAndTerminate()
{
    if (mGlfwWindow != nullptr)
    {
        glfwDestroyWindow(mGlfwWindow);
        glfwTerminate();
        mGlfwWindow = nullptr;
    }
}

bool Window::processWindowEvent()
{
    if (glfwWindowShouldClose(mGlfwWindow))
    {
        return true;
    }

    glfwPollEvents();

    return false;
}

Window::~Window()
{
    destroyAndTerminate();
}

} // namespace Bunny::Base
