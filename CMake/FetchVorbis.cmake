FetchContent_Declare(
  vorbis_content
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL https://github.com/xiph/vorbis/releases/download/v1.3.7/libvorbis-1.3.7.tar.xz
  URL_HASH MD5=50902641d358135f06a8392e61c9ac77
)

FetchContent_GetProperties(vorbis_content)
if(NOT vorbis_content_POPULATED)
  FetchContent_Populate(vorbis_content)
  file(COPY Engine/libsrc/vorbis/CMakeLists.txt DESTINATION ${vorbis_content_SOURCE_DIR})
  add_subdirectory(${vorbis_content_SOURCE_DIR} ${vorbis_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
set(VORBIS_LIBRARIES Vorbis::Vorbis)
