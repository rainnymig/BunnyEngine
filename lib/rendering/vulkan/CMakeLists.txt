add_library(VulkanRenderer 

)

target_sources(VulkanRenderer
    PUBLIC
        headers/VulkanRenderer.h
        headers/VulkanRendererFactory.h
        headers/QueueFamilyIndices.h
    PRIVATE
        src/VulkanRenderer.cpp
        src/VulkanRendererFactory.cpp    
)

find_package(Vulkan REQUIRED)

# target_include_directories(VulkanRenderer PUBLIC "../renderer")
target_include_directories(VulkanRenderer PUBLIC "./headers")
target_include_directories(VulkanRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}")

target_link_libraries(VulkanRenderer PUBLIC Renderer)

target_link_libraries(VulkanRenderer PRIVATE "${Vulkan_LIBRARIES}")
target_link_libraries(VulkanRenderer PRIVATE glfw)
