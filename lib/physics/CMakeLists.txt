add_library(PhysicsEngine)

target_sources(PhysicsEngine
    PUBLIC
        headers/Common.h
        headers/Vector3.h
    PRIVATE
        src/Vector3.cpp
)

target_include_directories(PhysicsEngine PUBLIC "./headers")
