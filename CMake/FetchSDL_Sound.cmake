FetchContent_Declare(
    sdlsound_content
    URL https://github.com/icculus/SDL_sound/archive/495e948b455af48eb45f75cccc060498f1e0e8a2.tar.gz
    URL_HASH MD5=d743aee2e550dca3a96b14d41e78b530
)

FetchContent_GetProperties(sdlsound_content)
if(NOT sdlsound_content)
  FetchContent_Populate(sdlsound_content)
  set(SDLSOUND_BUILD_SHARED off CACHE BOOL "no shared")
  set(SDLSOUND_BUILD_TEST off CACHE BOOL "no tests")
  set(SDLSOUND_BUILD_STATIC on CACHE BOOL "static")

  # see why we need to manually enable here: https://github.com/icculus/SDL_sound/issues/19#issuecomment-1079263491
  set(SDLSOUND_DECODER_MIDI on CACHE BOOL "")

  message("Including SDL2_sound ...")
  message("SDL2_DIR: ${SDL2_DIR}")
  message("sdl2_content_SOURCE_DIR: ${sdl2_content_SOURCE_DIR}")
  add_subdirectory(${sdlsound_content_SOURCE_DIR} ${sdlsound_content_BINARY_DIR} EXCLUDE_FROM_ALL)
  include_directories(${sdlsound_content_SOURCE_DIR}/src/)
  add_library(sdl2_sound-interface INTERFACE)
  target_link_libraries(sdl2_sound-interface INTERFACE SDL2_sound-static)
  add_library(SDL2_sound::SDL2_sound ALIAS sdl2_sound-interface)
  target_link_libraries(SDL2_sound-static  ${SDL2_LIBRARIES})
endif()