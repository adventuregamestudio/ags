FetchContent_Declare(
  ogg_content
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL https://github.com/xiph/ogg/releases/download/v1.3.5/libogg-1.3.5.tar.xz
  URL_HASH MD5=3178c98341559657a15b185bf5d700a5
)

FetchContent_GetProperties(ogg_content)
if(NOT ogg_content_POPULATED)
  FetchContent_Populate(ogg_content)
  file(COPY Engine/libsrc/ogg/CMakeLists.txt DESTINATION ${ogg_content_SOURCE_DIR})
  add_subdirectory(${ogg_content_SOURCE_DIR} ${ogg_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
set(OGG_LIBRARIES Ogg::Ogg)
