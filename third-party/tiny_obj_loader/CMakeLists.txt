add_library(TinyObjLoader)

target_sources(TinyObjLoader
    PUBLIC
        headers/tiny_obj_loader.h
    PRIVATE
        src/TinyObjLoaderImpl.cpp
)

target_include_directories(TinyObjLoader PUBLIC ./headers)
