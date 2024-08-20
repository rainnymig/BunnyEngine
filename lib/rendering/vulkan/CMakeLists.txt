add_library(VulkanRenderer 
    src/VulkanRenderer.cpp    
    headers/VulkanRenderer.h)

target_include_directories(VulkanRenderer PUBLIC "../includes")
target_include_directories(VulkanRenderer PUBLIC "./headers")

target_compile_features(VulkanRenderer PUBLIC cxx_std_20)
