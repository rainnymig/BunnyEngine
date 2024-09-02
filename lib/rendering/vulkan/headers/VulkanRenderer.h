#pragma once

#include <Renderer.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Bunny::Render
{
    class VulkanRenderer : public Renderer
    {
    public:
        virtual void render() override;



    private:
        GLFWwindow* mWindow;
    };
}