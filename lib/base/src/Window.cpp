#include "Window.h"

#include "Error.h"

namespace Bunny::Base
{
bool Window::initialize(int width, int height, bool isFullscreen, const std::string& name)
{
    //  Initialize the library
    if (!glfwInit())
    {
        return false;
    }

    //  Vulkan specific, see https://www.glfw.org/docs/3.3/vulkan_guide.html
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, isFullscreen ? GLFW_FALSE : GLFW_TRUE);

    mWindow = glfwCreateWindow(width, height, name.c_str(), isFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!mWindow)
    {
        glfwTerminate();
        PRINT_AND_RETURN_VALUE("Failed to create glfw window.", false)
    }

    //  Make the window's context current
    glfwMakeContextCurrent(mWindow);

    return true;
}

const VkResult Window::createSurface(
    VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) const
{
    return glfwCreateWindowSurface(instance, mWindow, allocator, surface);
}

void Window::getFrameBufferSize(int& width, int& height) const
{
    glfwGetFramebufferSize(mWindow, &width, &height);
}

void Window::destroyAndTerminate()
{
    if (mWindow != nullptr)
    {
        glfwDestroyWindow(mWindow);
        glfwTerminate();
        mWindow = nullptr;
    }
}

Window::~Window()
{
    destroyAndTerminate();
}

} // namespace Bunny::Base
