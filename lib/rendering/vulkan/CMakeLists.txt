add_library(VulkanRenderer 
    src/VulkanRenderer.cpp    
    headers/VulkanRenderer.h)

target_compile_features(VulkanRenderer PUBLIC cxx_std_20)

find_package(Vulkan REQUIRED)

target_link_libraries(VulkanRenderer PUBLIC Renderer)

# target_include_directories(VulkanRenderer PUBLIC "../renderer")
target_include_directories(VulkanRenderer PUBLIC "./headers")

target_link_libraries(VulkanRenderer PUBLIC Vulkan::Vulkan)
target_link_libraries(VulkanRenderer PRIVATE glfw)
