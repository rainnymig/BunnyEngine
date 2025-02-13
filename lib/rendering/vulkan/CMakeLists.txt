add_library(VulkanRenderer)

target_sources(VulkanRenderer
    PUBLIC
        headers/VulkanRendererFactory.h
        headers/BaseVulkanRenderer.h
    PRIVATE
        headers/Descriptor.h
        headers/ErrorCheck.h
        headers/Fundamentals.h
        headers/Helper.h
        headers/Material.h
        headers/Mesh.h
        headers/PipelineBuilder.h
        headers/QueueFamilyIndices.h
        headers/Renderable.h
        headers/Shader.h
        headers/SwapChainSupportDetails.h
        headers/Utils.h
        headers/Vertex.h
        headers/VulkanRenderer.h
        headers/VulkanRendererNext.h
        src/Descriptor.cpp
        src/Helper.cpp
        src/Material.cpp
        src/Mesh.cpp
        src/PipelineBuilder.cpp
        src/Renderable.cpp
        src/Shader.cpp
        src/Utils.cpp
        src/Vertex.cpp
        src/VulkanRenderer.cpp
        src/VulkanRendererFactory.cpp
        src/VulkanRendererNext.cpp
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
        fastgltf::fastgltf
    )
    