FetchContent_Declare(
    sdl2_content
    URL https://www.libsdl.org/release/SDL2-2.0.12.tar.gz
    URL_HASH MD5=783b6f2df8ff02b19bb5ce492b99c8ff
)

FetchContent_GetProperties(sdl2_content)
if(NOT sdl2_content_POPULATED)
    FetchContent_Populate(sdl2_content)
    if(ANDROID)
        set(SDL_SHARED ON CACHE BOOL "shared")
        set(SDL_STATIC ON CACHE BOOL "static")
        add_subdirectory(${sdl2_content_SOURCE_DIR} ${sdl2_content_BINARY_DIR} EXCLUDE_FROM_ALL)
        add_library(SDL2::SDL2 ALIAS SDL2)
        add_library(SDL2::SDL2main ALIAS SDL2main)
    elseif(WIN32 OR LINUX OR MACOS)
        set(SDL_SHARED ON CACHE BOOL "shared")
        set(SDL_STATIC ON CACHE BOOL "static")
       # set(SDL_STATIC_PIC ON CACHE BOOL "Static version of the library should be built with Position Independent Code")
        set(SDL_SHARED ON)
        set(SDL_STATIC ON)
        set(SDL_STATIC_PIC ON)
        set(FORCE_STATIC_VCRT ON CACHE BOOL "static windows static vcrc")
        add_subdirectory(${sdl2_content_SOURCE_DIR} ${sdl2_content_BINARY_DIR} EXCLUDE_FROM_ALL)
        add_library(SDL2::SDL2 ALIAS SDL2-static)
        add_library(SDL2::SDL2main ALIAS SDL2main)
    endif()
    include_directories("${sdl2_content_SOURCE_DIR}/include")

    if(NOT EXISTS "${sdl2_content_BINARY_DIR}/include/SDL2")
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${sdl2_content_SOURCE_DIR}/include ${sdl2_content_BINARY_DIR}/include/SDL2)
    endif()

    file(COPY CMake/Extra/sdl2-config.cmake DESTINATION ${sdl2_content_BINARY_DIR})
    set(SDL2_DIR ${sdl2_content_BINARY_DIR})
    list(APPEND SDL2_INCLUDE_DIRS "${sdl2_content_BINARY_DIR}/include/SDL2/")
    list(APPEND SDL2_INCLUDE_DIRS "${sdl2_content_BINARY_DIR}/include/")
    list(APPEND SDL2_LIBRARY_DIRS "${sdl2_content_BINARY_DIR}/")
    list(APPEND SDL2_LIBRARIES SDL2::SDL2)
    list(APPEND SDL2_LIBRARIES SDL2::SDL2main)
endif()

message("SDL2_LIBRARIES: ${SDL2_LIBRARIES}")
message("SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")
