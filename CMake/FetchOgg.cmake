FetchContent_Declare(
  ogg_content
  URL https://github.com/xiph/ogg/archive/31bd3f2707fb7dbae539a7093ba1fc4b2b37d84e.tar.gz
  URL_HASH MD5=b1236d9559e5dd5b68bc1de5999cdcc5
)

FetchContent_GetProperties(ogg_content)
if(NOT ogg_content_POPULATED)
  FetchContent_Populate(ogg_content)
  file(COPY Engine/libsrc/ogg/CMakeLists.txt DESTINATION ${ogg_content_SOURCE_DIR})
  add_subdirectory(${ogg_content_SOURCE_DIR} ${ogg_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
