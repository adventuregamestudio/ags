FetchContent_Declare(
  vorbis_content
  GIT_REPOSITORY https://github.com/xiph/vorbis.git
  GIT_TAG        9eadeccdc4247127d91ac70555074239f5ce3529
  GIT_SHALLOW    yes
)

FetchContent_GetProperties(vorbis_content)
if(NOT vorbis_content_POPULATED)
  FetchContent_Populate(vorbis_content)
  file(COPY Engine/libsrc/vorbis/CMakeLists.txt DESTINATION ${vorbis_content_SOURCE_DIR})
  add_subdirectory(${vorbis_content_SOURCE_DIR} ${vorbis_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
