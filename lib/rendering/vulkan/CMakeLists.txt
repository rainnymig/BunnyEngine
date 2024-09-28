add_library(VulkanRenderer)

target_sources(VulkanRenderer
    PUBLIC
        headers/VulkanRenderer.h
        headers/VulkanRendererFactory.h
        headers/Vertex.h
    PRIVATE
        headers/QueueFamilyIndices.h
        headers/SwapChainSupportDetails.h
        headers/Helper.h
        src/StbImageImpl.cpp
        src/TinyObjLoaderImpl.cpp
        src/Helper.cpp
        src/VulkanRenderer.cpp
        src/VulkanRendererFactory.cpp
)

find_package(Vulkan REQUIRED)

target_compile_definitions(VulkanRenderer PUBLIC GLFW_INCLUDE_VULKAN)

# target_include_directories(VulkanRenderer PUBLIC "../renderer")
target_include_directories(VulkanRenderer PUBLIC "./headers")
target_include_directories(VulkanRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}")
target_include_directories(VulkanRenderer PRIVATE "${CMAKE_SOURCE_DIR}/third-party/stb_image")
target_include_directories(VulkanRenderer PRIVATE "${CMAKE_SOURCE_DIR}/third-party/tiny_obj_loader")

target_link_libraries(VulkanRenderer PUBLIC Renderer)

target_link_libraries(VulkanRenderer PRIVATE "${Vulkan_LIBRARIES}")
target_link_libraries(VulkanRenderer PRIVATE glfw)
