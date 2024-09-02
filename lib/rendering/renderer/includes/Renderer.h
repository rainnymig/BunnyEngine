#pragma once

#include <GLFW/glfw3.h>
#include <memory>

namespace Bunny::Render
{
    class Renderer
    {
    public:
        virtual void render() = 0;
    };

    class RendererFactory
    {
    public:
        virtual std::unique_ptr<Renderer> makeRenderer(GLFWwindow* window) = 0;
    };
}
