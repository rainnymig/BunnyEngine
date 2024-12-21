add_library(VulkanRenderer)

target_sources(VulkanRenderer
    PUBLIC
        headers/VulkanRendererFactory.h
    PRIVATE
        headers/Camera.h
        headers/Descriptor.h
        headers/ErrorCheck.h
        headers/Fundamentals.h
        headers/Helper.h
        headers/Material.h
        headers/Mesh.h
        headers/PipelineBuilder.h
        headers/QueueFamilyIndices.h
        headers/Shader.h
        headers/SwapChainSupportDetails.h
        headers/Utils.h
        headers/Vertex.h
        headers/VulkanRenderer.h
        src/Camera.cpp
        src/Descriptor.cpp
        src/Helper.cpp
        src/Mesh.cpp
        src/PipelineBuilder.cpp
        src/Shader.cpp
        src/Utils.cpp
        src/VulkanRenderer.cpp
        src/VulkanRendererFactory.cpp
)

target_compile_definitions(VulkanRenderer PUBLIC GLFW_INCLUDE_VULKAN)

target_include_directories(VulkanRenderer PUBLIC "./headers")
target_include_directories(VulkanRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}")

target_link_libraries(VulkanRenderer 
    PUBLIC 
        Renderer
    PRIVATE
        "${Vulkan_LIBRARIES}"
        StbImage
        TinyObjLoader
        glfw
        vk-bootstrap::vk-bootstrap
        fmt::fmt
        vma
        imgui
    )
    