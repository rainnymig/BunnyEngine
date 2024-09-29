add_library(StbImage)

target_sources(StbImage
    PUBLIC
        headers/stb_image.h
    PRIVATE
        src/StbImageImpl.cpp
)

target_include_directories(StbImage PUBLIC ./headers)
