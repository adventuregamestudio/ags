FetchContent_Declare(
    sdlsound_content
    URL https://hg.icculus.org/icculus/SDL_sound/archive/4b78007f035f.tar.gz
    URL_HASH MD5=ff2448f5a789ae2a07636c243d8f8780
)

FetchContent_GetProperties(sdlsound_content)
if(NOT sdlsound_content)
  FetchContent_Populate(sdlsound_content)
  set(SDLSOUND_BUILD_SHARED off CACHE BOOL "no shared")
  set(SDLSOUND_BUILD_TEST off CACHE BOOL "no tests")
  set(SDLSOUND_BUILD_STATIC on CACHE BOOL "static")
  message("Including SDL2_sound ...")
  message("SDL2_DIR: ${SDL2_DIR}")
  message("sdl2_content_SOURCE_DIR: ${sdl2_content_SOURCE_DIR}")
  add_subdirectory(${sdlsound_content_SOURCE_DIR} ${sdlsound_content_BINARY_DIR} EXCLUDE_FROM_ALL)
  include_directories(${sdlsound_content_SOURCE_DIR}/src/)
  add_library(sdl2_sound-interface INTERFACE)
  add_dependencies(sdl2_sound-interface ${sdl2_content_SOURCE_DIR})
  target_link_libraries(sdl2_sound-interface INTERFACE SDL2_sound-static)
  add_library(SDL2_sound::SDL2_sound ALIAS sdl2_sound-interface)
  target_link_libraries(SDL2_sound-static  ${SDL2_LIBRARIES})
endif()
