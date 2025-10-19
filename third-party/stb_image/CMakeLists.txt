add_library(StbImage)

target_sources(StbImage
    PUBLIC
        headers/stb_image.h
        headers/stb_image_write.h
    PRIVATE
        src/StbImageImpl.cpp
        src/StbImageWriteImpl.cpp
)

target_include_directories(StbImage PUBLIC ./headers)
