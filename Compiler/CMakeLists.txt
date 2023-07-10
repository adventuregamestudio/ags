add_library(compiler)

set_target_properties(compiler PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_EXTENSIONS NO
        )

target_include_directories(compiler PUBLIC .)
target_include_directories(compiler PUBLIC ../Common)
set(COMPILER_COMMON_SOURCES
        ../Common/debug/debugmanager.cpp
        ../Common/script/cc_common.cpp
        ../Common/script/cc_script.cpp
        ../Common/script/cc_reflect.cpp
        ../Common/util/bufferedstream.cpp
        ../Common/util/cmdlineopts.cpp
        ../Common/util/data_ext.cpp
        ../Common/util/datastream.cpp
        ../Common/util/directory.cpp
        ../Common/util/file.cpp
        ../Common/util/path.cpp
        ../Common/util/filestream.cpp
        ../Common/util/stdio_compat.c
        ../Common/util/stream.cpp
        ../Common/util/string.cpp
        ../Common/util/string_compat.c
        ../Common/util/string_utils.cpp
        ../Common/util/textstreamreader.cpp
        ../Common/util/version.cpp)

set(COMPILER_SOURCES
        fmem.cpp
        fmem.h
        script/cc_compiledscript.cpp
        script/cc_compiledscript.h
        script/cc_internallist.cpp
        script/cc_internallist.h
        script/cc_macrotable.cpp
        script/cc_macrotable.h
        script/cc_symboltable.cpp
        script/cc_symboltable.h
        script/cc_symboldef.h
        script/cc_treemap.cpp
        script/cc_treemap.h
        script/cc_variablesymlist.h
        script/cs_compiler.cpp
        script/cs_compiler.h
        script/cs_parser.cpp
        script/cs_parser.h
        script/cs_parser_common.cpp
        script/cs_parser_common.h
        preproc/preprocessor.cpp
        preproc/preprocessor.h)

target_sources(compiler PRIVATE
        ${COMPILER_SOURCES}
        ${COMPILER_COMMON_SOURCES})

if (WIN32)
    target_link_libraries(compiler PUBLIC shlwapi)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${COMPILER_SOURCES})

add_library(AGS::Compiler ALIAS compiler)

add_executable(agscc main.cpp compiler.cpp compiler.h)
set_target_properties(agscc PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_EXTENSIONS NO
        )

target_link_libraries(agscc PUBLIC AGS::Compiler)

if (AGS_DESKTOP)
    install(TARGETS agscc RUNTIME DESTINATION bin)
endif ()

if(AGS_TESTS)
    add_executable(
            compiler_test
            test/cc_internallist_test.cpp
            test/cc_symboltable_test.cpp
            test/cc_treemap_test.cpp
            test/cs_parser_test.cpp
            test/preprocessor_test.cpp
            test/cc_test_helper.cpp
            test/cc_test_helper.h
    )
    set_target_properties(compiler_test PROPERTIES
            CXX_STANDARD 11
            CXX_EXTENSIONS NO
            C_STANDARD 11
            C_EXTENSIONS NO
            INTERPROCEDURAL_OPTIMIZATION FALSE
            )
    target_link_libraries(
            compiler_test
            compiler
            gtest_main
    )

    include(GoogleTest)
    gtest_add_tests(TARGET compiler_test)
endif()