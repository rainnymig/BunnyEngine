#pragma once

#include <Renderer.h>

namespace Bunny::Render
{
    class VulkanRendererFactory : public RendererFactory
    {
    public:
        virtual std::unique_ptr<Renderer> makeRenderer(GLFWwindow* window) override;
    };
}