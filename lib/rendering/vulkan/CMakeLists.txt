add_library(VulkanRenderer)

target_sources(VulkanRenderer
    PUBLIC
        headers/AccelerationStructureBuilder.h
        headers/AccelerationStructureData.h
        headers/ComputePipelineBuilder.h
        headers/CullingPass.h
        headers/DeferredShadingPass.h
        headers/DepthReducePass.h
        headers/Descriptor.h
        headers/ErrorCheck.h
        headers/FinalOutputPass.h
        headers/Fundamentals.h
        headers/GBufferPass.h
        headers/GraphicsPipelineBuilder.h
        headers/Helper.h
        headers/Material.h
        headers/MaterialBank.h
        headers/MeshBank.h
        headers/OceanPass.h
        headers/PbrForwardPass.h
        headers/PbrGraphicsPass.h
        headers/RaytracingPipelineBuilder.h
        headers/RaytracingShadowPass.h
        headers/Shader.h
        headers/ShaderData.h
        headers/SkyPass.h
        headers/SwapChainSupportDetails.h
        headers/TextureBank.h
        headers/TexturePreviewPass.h
        headers/Vertex.h
        headers/VulkanGraphicsRenderer.h
        headers/VulkanRenderResources.h
        headers/WaveSpectrumPrePass.h
    PRIVATE
        src/AccelerationStructureBuilder.cpp
        src/ComputePipelineBuilder.cpp
        src/CullingPass.cpp
        src/DeferredShadingPass.cpp
        src/DepthReducePass.cpp
        src/Descriptor.cpp
        src/FinalOutputPass.cpp
        src/Helper.cpp
        src/Material.cpp
        src/MaterialBank.cpp
        src/MeshBank.cpp
        src/OceanPass.cpp
        src/PbrForwardPass.cpp
        src/PbrGraphicsPass.cpp
        src/RaytracingPipelineBuilder.cpp
        src/RaytracingShadowPass.cpp
        src/GBufferPass.cpp
        src/GraphicsPipelineBuilder.cpp
        src/Shader.cpp
        src/SkyPass.cpp
        src/TextureBank.cpp
        src/TexturePreviewPass.cpp
        src/Vertex.cpp
        src/VulkanGraphicsRenderer.cpp
        src/VulkanRenderResources.cpp
        src/WaveSpectrumPrePass.cpp
)

# target_compile_definitions(VulkanRenderer PUBLIC GLFW_INCLUDE_VULKAN)

target_include_directories(VulkanRenderer PUBLIC "./headers")
# target_include_directories(VulkanRenderer PUBLIC "${Vulkan_INCLUDE_DIRS}")

target_link_libraries(VulkanRenderer 
    PUBLIC 
        Renderer
        vma
        vk-bootstrap::vk-bootstrap
        volk
    PRIVATE
        # "${Vulkan_LIBRARIES}"
        StbImage
        TinyObjLoader
        glfw
        fmt::fmt
        imgui
        glm
    )
    
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS GLM_FORCE_RIGHT_HANDED)
