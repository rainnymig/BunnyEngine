add_library(Renderer)

target_sources(Renderer
    PUBLIC
        headers/Camera.h
        headers/Light.h
        headers/Renderer.h
    PRIVATE
        src/Camera.cpp
        src/Light.cpp
)

target_include_directories(Renderer PUBLIC ./headers)

target_link_libraries(Renderer PUBLIC glm Base)

add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE GLM_FORCE_RADIANS)
