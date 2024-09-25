add_library(StbImage)

target_sources(StbImage
    PUBLIC
        stb_image.h
)

target_include_directories(StbImage PUBLIC "./")

set_target_properties(StbImage PROPERTIES LINKER_LANGUAGE CXX)