add_library(PhysicsEngine)

target_sources(PhysicsEngine
    PUBLIC
        headers/Common.h
        headers/Vector3.h
        headers/MassPoint.h
    PRIVATE
        src/Vector3.cpp
        src/MassPoint.cpp
)

target_include_directories(PhysicsEngine PUBLIC "./headers")
