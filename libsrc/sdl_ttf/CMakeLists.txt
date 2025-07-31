add_library(sdl_ttf)

set_target_properties(sdl_ttf PROPERTIES
        C_STANDARD 11
        C_EXTENSIONS NO
        )

target_include_directories(sdl_ttf PUBLIC . )

target_sources(sdl_ttf
        PRIVATE
        SDL_ttf.c
        SDL_ttf.h
        )

target_link_libraries(sdl_ttf PUBLIC freetype)
target_link_libraries(sdl_ttf PUBLIC ${SDL2_LIBRARY})

add_library(SDL_ttf::SDL_ttf ALIAS sdl_ttf)