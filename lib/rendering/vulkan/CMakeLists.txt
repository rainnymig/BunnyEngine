add_library(VulkanRenderer)

target_sources(VulkanRenderer
    PUBLIC
        headers/Camera.h
        headers/Descriptor.h
        headers/ErrorCheck.h
        headers/Fundamentals.h
        headers/Material.h
        headers/Mesh.h
        headers/PipelineBuilder.h
        headers/Shader.h
        headers/VulkanRenderer.h
        headers/VulkanRendererFactory.h
        headers/Vertex.h
    PRIVATE
        headers/Helper.h
        headers/QueueFamilyIndices.h
        headers/SwapChainSupportDetails.h
        headers/Utils.h
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

find_package(Vulkan REQUIRED)

target_compile_definitions(VulkanRenderer PUBLIC GLFW_INCLUDE_VULKAN)

# target_include_directories(VulkanRenderer PUBLIC "../renderer")
target_include_directories(VulkanRenderer PUBLIC "./headers")
target_include_directories(VulkanRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}")

target_link_libraries(VulkanRenderer 
    PUBLIC 
        Renderer
        vma
    PRIVATE
        "${Vulkan_LIBRARIES}"
        StbImage
        TinyObjLoader
        glfw
        vk-bootstrap::vk-bootstrap
        fmt::fmt
    )

# target_link_libraries(VulkanRenderer PRIVATE "${Vulkan_LIBRARIES}")

# target_link_libraries(VulkanRenderer PRIVATE StbImage)
# target_link_libraries(VulkanRenderer PRIVATE TinyObjLoader)
# target_link_libraries(VulkanRenderer PRIVATE glfw)
# target_link_libraries(VulkanRenderer PRIVATE vk-bootstrap::vk-bootstrap)
