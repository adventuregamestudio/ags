FetchContent_Declare(
  theora_content
  GIT_REPOSITORY https://github.com/xiph/theora.git
  GIT_TAG        e5d205bfe849f1b41f45b91a0b71a3bdc6cd458f
)

FetchContent_GetProperties(theora_content)
if(NOT theora_content_POPULATED)
  FetchContent_Populate(theora_content)
  file(COPY Engine/libsrc/theora/CMakeLists.txt DESTINATION ${theora_content_SOURCE_DIR})
  add_subdirectory(${theora_content_SOURCE_DIR} ${theora_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
