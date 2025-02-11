add_library(Base)

target_sources(Base
    PUBLIC
        headers/Error.h
        headers/Transform.h
    PRIVATE
        src/Transform.cpp
)

target_include_directories(Base PUBLIC ./headers)

target_link_libraries(Base PUBLIC glm)
