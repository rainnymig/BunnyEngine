add_library(Base)

target_sources(Base
    PUBLIC
        headers/BunnyGuard.h
        headers/BunnyResult.h
        headers/Error.h
        headers/FunctionStack.h
        headers/Transform.h
        headers/Window.h
    PRIVATE
        src/Transform.cpp
        src/Window.cpp
)

target_include_directories(Base PUBLIC ./headers)
target_include_directories(Base PUBLIC "${Vulkan_INCLUDE_DIRS}")

target_link_libraries(Base 
    PUBLIC 
        glm 
        glfw 
        fmt::fmt 
        Vulkan::Vulkan
)
