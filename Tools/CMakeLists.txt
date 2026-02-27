add_library(libtools)

set_target_properties(libtools PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_EXTENSIONS NO
        )

target_include_directories(libtools PUBLIC .)

# we could link to AGS::Common, but this provides much faster LTO times with Release builds
target_include_directories(libtools PUBLIC ../Common)
set(TOOLS_COMMON_SOURCES
        ../Common/ac/spritefile.cpp
        ../Common/ac/spritefile.h
        ../Common/ac/wordsdictionary.cpp
        ../Common/ac/wordsdictionary.h
        ../Common/data/asset.cpp
        ../Common/data/asset.h
        ../Common/data/data_helpers.cpp
        ../Common/data/data_helpers.h
        ../Common/data/data_ext.cpp
        ../Common/data/data_ext.h
        ../Common/data/multifilelib.cpp
        ../Common/data/multifilelib.h
        ../Common/data/tra_file.cpp
        ../Common/data/tra_file.h
        ../Common/debug/debugmanager.cpp
        ../Common/debug/debugmanager.h
        ../Common/game/room_file_base.cpp
        ../Common/gfx/bitmapdata.cpp
        ../Common/gfx/bitmapdata.h
        ../Common/gfx/image_file.cpp
        ../Common/gfx/image_file.h
        ../Common/util/bufferedstream.cpp
        ../Common/util/bufferedstream.h
        ../Common/util/cmdlineopts.cpp
        ../Common/util/cmdlineopts.h
        ../Common/util/compress.cpp
        ../Common/util/compress.h
        ../Common/util/directory.cpp
        ../Common/util/directory.h
        ../Common/util/file.cpp
        ../Common/util/file.h
        ../Common/util/filestream.cpp
        ../Common/util/filestream.h
        ../Common/util/lzw.cpp
        ../Common/util/lzw.h
        ../Common/util/memorystream.cpp
        ../Common/util/memorystream.h
        ../Common/util/path.cpp
        ../Common/util/path.h
        ../Common/util/stdio_compat.c
        ../Common/util/stdio_compat.h
        ../Common/util/stream.cpp
        ../Common/util/stream.h
        ../Common/util/string.cpp
        ../Common/util/string.h
        ../Common/util/string_compat.c
        ../Common/util/string_compat.h
        ../Common/util/string_utils.cpp
        ../Common/util/string_utils.h
        ../Common/util/textstreamreader.cpp
        ../Common/util/textstreamreader.h)


#-----------------------------------------------------------------------------#
# Minimal Allegro config
#-----------------------------------------------------------------------------#

add_library(miniallegro)

target_include_directories(miniallegro PUBLIC ../libsrc/allegro/include)
target_compile_definitions(miniallegro PRIVATE ALLEGRO_STATICLINK ALLEGRO_SRC)
target_sources(miniallegro
        PRIVATE
        ../libsrc/allegro/src/color.c
)

# for some reason I can't figure it out, if I don't do this I get link errors
# I am using C++ mode from looking into how alconfig.h works
set_source_files_properties(
        ../libsrc/allegro/src/color.c
        PROPERTIES LANGUAGE CXX
)

#-----------------------------------------------------------------------------#

target_sources(libtools
        PRIVATE
        data/agfreader.cpp
        data/agfreader.h
        data/dialogscriptconv.cpp
        data/dialogscriptconv.h
        data/game_utils.h
        data/include_utils.cpp
        data/include_utils.h
        data/mfl_utils.cpp
        data/mfl_utils.h
        data/room_utils.cpp
        data/room_utils.h
        data/script_utils.cpp
        data/script_utils.h
        data/scriptgen.cpp
        data/scriptgen.h
        data/tra_utils.cpp
        data/tra_utils.h
        ${TOOLS_COMMON_SOURCES}
        )

target_link_libraries(libtools PUBLIC miniallegro)
target_link_libraries(libtools PUBLIC MiniZ::MiniZ)
target_link_libraries(libtools PUBLIC TinyXML2::TinyXML2)
if (WIN32)
    target_link_libraries(libtools PUBLIC shlwapi)
endif()

#----- agf2dlgasc ---------------------------------------------
add_executable(agf2dlgasc agf2dlgasc/main.cpp)
set_target_properties(agf2dlgasc PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )
target_link_libraries(agf2dlgasc PUBLIC libtools)

#----- agfexport -----------------------------------------------
add_executable(agfexport agfexport/main.cpp)
set_target_properties(agfexport PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )
target_link_libraries(agfexport PUBLIC libtools)

#----- agspak -------------------------------------------------
add_executable(agspak agspak/main.cpp
        agspak/commands.h
        agspak/commands.cpp
        )
set_target_properties(agspak PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )
target_link_libraries(agspak PUBLIC libtools)

#----- crm2ash ------------------------------------------------
add_executable(crm2ash crm2ash/main.cpp)
set_target_properties(crm2ash PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )
target_link_libraries(crm2ash PUBLIC libtools)

#----- crmpak -------------------------------------------------
add_executable(crmpak crmpak/main.cpp)
set_target_properties(crmpak PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )
target_link_libraries(crmpak PUBLIC libtools)

#----- spritepak -------------------------------------------------
add_executable(spritepak spritepak/main.cpp
        spritepak/commands.h
        spritepak/commands.cpp
        )
set_target_properties(spritepak PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )
target_link_libraries(spritepak PUBLIC libtools)

#----- trac ---------------------------------------------------
add_executable(trac trac/main.cpp)
set_target_properties(trac PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        )
target_link_libraries(trac PUBLIC libtools)

list(APPEND TOOLS_TARGETS
        agf2dlgasc
        agfexport
        agspak
        crm2ash
        crmpak
        spritepak
        trac
        )

# Bundle-like target to build all tools
add_custom_target(Tools)
add_dependencies(Tools ${TOOLS_TARGETS})

if (AGS_DESKTOP)
    install(TARGETS ${TOOLS_TARGETS} RUNTIME DESTINATION bin)
endif ()

if(AGS_TESTS)
   add_executable(tools_test
      test/data_helpers_test.cpp
      test/include_utils_test.cpp
   )
   set_target_properties(tools_test PROPERTIES
      CXX_STANDARD 11
      CXX_EXTENSIONS NO
      C_STANDARD 11
      C_EXTENSIONS NO
      INTERPROCEDURAL_OPTIMIZATION FALSE
   )
   target_link_libraries(tools_test
      libtools
      gtest_main
   )

   include(GoogleTest)
   gtest_add_tests(TARGET tools_test)
endif()