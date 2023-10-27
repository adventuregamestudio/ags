FetchContent_Declare(
  theora_content
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL https://github.com/xiph/theora/archive/7180717276af1ebc7da15c83162d6c5d6203aabf.tar.gz
  URL_HASH MD5=5f1c0b5efdec0f9621bc59ba89e6652b
)

FetchContent_GetProperties(theora_content)
if(NOT theora_content_POPULATED)
  FetchContent_Populate(theora_content)
  file(COPY Engine/libsrc/theora/CMakeLists.txt DESTINATION ${theora_content_SOURCE_DIR})
  add_subdirectory(${theora_content_SOURCE_DIR} ${theora_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
set(THEORA_LIBRARIES Theora::TheoraDecoder)
