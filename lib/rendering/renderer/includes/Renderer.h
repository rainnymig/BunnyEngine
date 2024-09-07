#pragma once

#include <GLFW/glfw3.h>
#include <memory>

namespace Bunny::Render
{
class Renderer
{
  public:
    virtual void initialize() = 0;
    virtual void render() = 0;
    virtual void cleanup() = 0;
};

class RendererFactory
{
  public:
    virtual std::unique_ptr<Renderer> makeRenderer(GLFWwindow* window) = 0;
};
} // namespace Bunny::Render
