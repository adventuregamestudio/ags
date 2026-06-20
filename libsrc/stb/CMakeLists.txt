add_library(stb)

set_target_properties(stb PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )

target_include_directories(stb PUBLIC . )

target_sources(stb
        PRIVATE
        stb_image.c
        stb_image.h
        )

add_library(stb::stb ALIAS stb)