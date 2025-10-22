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

set(PREPROCESSOR_SOURCES
        preproc/cc_macrotable.cpp
        preproc/cc_macrotable.h
        preproc/preprocessor.cpp
        preproc/preprocessor.h
)

set(LEGACY_COMPILER_SOURCES
        fmem.cpp
        fmem.h
        script/cc_compiledscript.cpp
        script/cc_compiledscript.h
        script/cc_internallist.cpp
        script/cc_internallist.h
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
        script/cs_parser_common.h
        ${PREPROCESSOR_SOURCES})

set(LEGACY_COMPILER_TEST_SOURCES
        test/cc_internallist_test.cpp
        test/cc_symboltable_test.cpp
        test/cc_treemap_test.cpp
        test/cs_parser_test.cpp
        test/preprocessor_test.cpp
        test/cc_test_helper.cpp
        test/cc_test_helper.h)

set(NEW_COMPILER_SOURCES
        script2/cc_compiledscript.cpp
        script2/cc_compiledscript.h
        script2/cc_internallist.cpp
        script2/cc_internallist.h
        script2/cc_symboltable.cpp
        script2/cc_symboltable.h
        script2/cs_compile_time.cpp
        script2/cs_compile_time.h
        script2/cs_compiler.cpp
        script2/cs_compiler.h
        script2/cs_message_handler.cpp
        script2/cs_message_handler.h
        script2/cs_parser.cpp
        script2/cs_parser.h
        script2/cs_parser_common.h
        script2/cs_scanner.cpp
        script2/cs_scanner.h
        ${PREPROCESSOR_SOURCES})

set(NEW_COMPILER_TEST_SOURCES
        test2/cc_bytecode_test_0.cpp
        test2/cc_bytecode_test_1.cpp
        test2/cc_bytecode_test_lib.cpp
        test2/cc_bytecode_test_lib.h
        test2/cc_internallist_test.cpp
        test2/cc_parser_test_0.cpp
        test2/cc_parser_test_1.cpp
        test2/cc_parser_test_2.cpp
        test2/cc_parser_test_lib.cpp
        test2/cc_parser_test_lib.h
        test2/cc_scanner_test.cpp
        test2/cc_symboltable_test.cpp)

if(AGS_BUILD_LEGACY_COMPILER)
    target_compile_definitions(compiler PRIVATE
            LEGACY_COMPILER=1
    )
    set(COMPILER_SOURCES ${LEGACY_COMPILER_SOURCES})
    set(COMPILER_TEST_SOURCES ${LEGACY_COMPILER_TEST_SOURCES})
else ()
    set(COMPILER_SOURCES ${NEW_COMPILER_SOURCES})
    set(COMPILER_TEST_SOURCES ${NEW_COMPILER_TEST_SOURCES})
endif ()

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
            ${COMPILER_TEST_SOURCES}
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
    if(AGS_USE_LOCAL_GTEST)
        target_link_libraries(compiler_test gtest)
    endif()

    include(GoogleTest)
    gtest_add_tests(TARGET compiler_test)
endif()
