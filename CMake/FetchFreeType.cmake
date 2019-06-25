# Grab freetype from the lib-alfont fork we maintain.

FetchContent_Declare(
  freetype_content
  GIT_REPOSITORY https://github.com/adventuregamestudio/lib-alfont.git
  GIT_TAG        alfont-1.9.1-agspatch
  GIT_SHALLOW    yes
)

FetchContent_GetProperties(freetype_content)
if(NOT freetype_content_POPULATED)
  FetchContent_Populate(freetype_content)
  file(COPY Engine/libsrc/freetype-2.1.3/CMakeLists.txt DESTINATION ${freetype_content_SOURCE_DIR}/freetype)
  add_subdirectory(${freetype_content_SOURCE_DIR}/freetype ${freetype_content_BINARY_DIR}/freetype EXCLUDE_FROM_ALL)
endif()
