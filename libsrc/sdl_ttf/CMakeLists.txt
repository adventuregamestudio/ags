add_library(sdl_ttf)

set_target_properties(sdl_ttf PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS NO
        )

target_include_directories(sdl_ttf PUBLIC . )

target_sources(sdl_ttf
        PRIVATE
        sdl_ttf.c
        sdl_ttf.h
        )

add_library(SDL_ttf::SDL_ttf ALIAS sdl_ttf)