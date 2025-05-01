if(LINUX AND CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
    set(LINUX_OLD_GCC TRUE)
else()
    set(LINUX_OLD_GCC FALSE)
endif()

message("googletest building with LINUX_OLD_GCC:" ${LINUX_OLD_GCC})

# When we drop compatibility with GCC 4.8 and below, we can keep compatibility only with newer Googletest
# Then we can use MakeAvailable instead of Populate here

FetchContent_GetProperties(googletest_content)
if(NOT googletest_content_POPULATED)
    if(LINUX_OLD_GCC)
        FetchContent_Populate(googletest_content
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
                URL https://github.com/google/googletest/archive/refs/tags/release-1.8.1.tar.gz
                URL_HASH MD5=2e6fbeb6a91310a16efe181886c59596
        )
    else()
        FetchContent_Populate(googletest_content
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
                URL https://github.com/google/googletest/archive/52204f78f94d7512df1f0f3bea1d47437a2c3a58.tar.gz
                URL_HASH MD5=9512a106bb006ab84e0a822ec363c6c7
        )
    endif()
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    add_subdirectory(${googletest_content_SOURCE_DIR} ${googletest_content_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

if(LINUX_OLD_GCC)
    target_compile_options(gtest PRIVATE -Wno-undef -Wno-missing-noreturn -Wno-inline)
    target_compile_options(gtest_main PRIVATE -Wno-undef -Wno-missing-noreturn -Wno-inline)
    # this is because we are building an old googletest before gmock was absorbed by it
    target_compile_options(gmock PRIVATE -Wno-undef -Wno-missing-noreturn -Wno-inline)
    target_compile_options(gmock_main PRIVATE -Wno-undef -Wno-missing-noreturn -Wno-inline)
endif()