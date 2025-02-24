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

    mWindow = glfwCreateWindow(width, height, name.c_str(), isFullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (!mWindow)
    {
        glfwTerminate();
        PRINT_AND_RETURN_VALUE("Failed to create glfw window.", BUNNY_SAD)
    }

    //  Make the window's context current
    glfwMakeContextCurrent(mWindow);

    return BUNNY_HAPPY;
}

BunnyResult Window::createSurface(
    VkInstance instance, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) const
{
    VkResult result = glfwCreateWindowSurface(instance, mWindow, allocator, surface);

    return result == VK_SUCCESS ? BUNNY_HAPPY : BUNNY_SAD;
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

bool Window::processWindowEvent()
{
    if (glfwWindowShouldClose(mWindow))
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
