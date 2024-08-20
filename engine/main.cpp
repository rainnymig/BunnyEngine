#include "VulkanRenderer.h"

#include <iostream>
#include <memory>

int main() {
    std::cout << "Welcome to Bunny Engine!\n";

    std::unique_ptr<Bunny::Render::Renderer> renderer = std::make_unique<Bunny::Render::VulkanRenderer>();
    renderer->render();
    
}