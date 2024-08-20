#pragma once

#include <Renderer.h>

namespace Bunny::Render
{
    class VulkanRenderer : public Renderer
    {
    public:
        virtual void render() override;
    };
}