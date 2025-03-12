add_library(VulkanRenderer)

target_sources(VulkanRenderer
    PUBLIC
        headers/BaseVulkanRenderer.h
        headers/Culling.h
        headers/Descriptor.h
        headers/ErrorCheck.h
        headers/Fundamentals.h
        headers/Helper.h
        headers/Material.h
        headers/MaterialBank.h
        headers/Mesh.h
        headers/MeshBank.h
        headers/PipelineBuilder.h
        headers/QueueFamilyIndices.h
        headers/Renderable.h
        headers/RenderJob.h
        headers/RenderPass.h
        headers/Shader.h
        headers/ShaderData.h
        headers/SwapChainSupportDetails.h
        headers/TextureBank.h
        headers/Vertex.h
        headers/VulkanGraphicsRenderer.h
        headers/VulkanRenderer.h
        headers/VulkanRendererNext.h
        headers/VulkanRendererFactory.h
        headers/VulkanRenderResources.h
    PRIVATE
        src/BaseVulkanRenderer.cpp
        src/Culling.cpp
        src/Descriptor.cpp
        src/Helper.cpp
        src/Material.cpp
        src/MaterialBank.cpp
        src/Mesh.cpp
        src/MeshBank.cpp
        src/PipelineBuilder.cpp
        src/Renderable.cpp
        src/RenderJob.cpp
        src/RenderPass.cpp
        src/Shader.cpp
        src/Vertex.cpp
        src/VulkanGraphicsRenderer.cpp
        src/VulkanRenderer.cpp
        src/VulkanRendererFactory.cpp
        src/VulkanRendererNext.cpp
        src/VulkanRenderResources.cpp
)

target_compile_definitions(VulkanRenderer PUBLIC GLFW_INCLUDE_VULKAN)

target_include_directories(VulkanRenderer PUBLIC "./headers")
target_include_directories(VulkanRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}")

target_link_libraries(VulkanRenderer 
    PUBLIC 
        Renderer
        vma
        vk-bootstrap::vk-bootstrap
    PRIVATE
        "${Vulkan_LIBRARIES}"
        StbImage
        TinyObjLoader
        glfw
        fmt::fmt
        imgui
        fastgltf::fastgltf
    )
    
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)
