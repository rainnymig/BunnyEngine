add_library(PhysicsEngine)

target_sources(PhysicsEngine
    PUBLIC
        headers/Common.h
        headers/Vector.h
        headers/MassPoint.h
    PRIVATE
        src/Vector.cpp
        src/MassPoint.cpp
)

target_include_directories(PhysicsEngine PUBLIC "./headers")

target_link_libraries(PhysicsEngine PUBLIC glm)