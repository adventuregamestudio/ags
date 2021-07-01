FetchContent_Declare(
    googletest_content
    URL https://github.com/google/googletest/archive/4ec4cd23f486bf70efcc5d2caa40f24368f752e3.tar.gz
    URL_HASH MD5=b907483a9045a2edda15ee7d2a68aaa5
)

FetchContent_GetProperties(googletest_content)
if(NOT googletest_content_POPULATED)
    FetchContent_Populate(googletest_content)
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${googletest_content_SOURCE_DIR} ${googletest_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()