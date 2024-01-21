FetchContent_Declare(
    sdlsound_content
    URL https://github.com/icculus/SDL_sound/archive/1f8b5c25fabe1d611e828f112b7c76dac907b524.tar.gz
    URL_HASH SHA1=1deb0832f449eebdee313b48d67ce15d54df8241
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
endif()
