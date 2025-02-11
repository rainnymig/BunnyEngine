add_library(vma)

set_target_properties(vma PROPERTIES LINKER_LANGUAGE CXX)

target_sources(vma
    PUBLIC
        headers/vk_mem_alloc.h
    PRIVATE
        src/vk_mem_alloc.cpp
)

target_include_directories(vma PRIVATE "${Vulkan_INCLUDE_DIRS}")

target_include_directories(vma PUBLIC ./headers)