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
        ../Common/ac/characterinfo.cpp
        ../Common/ac/inventoryiteminfo.cpp
        ../Common/ac/mousecursor.cpp
        ../Common/ac/wordsdictionary.cpp
        ../Common/ac/wordsdictionary.h
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
		../Common/gfx/image_bmp.cpp
        ../Common/gfx/image_file.cpp
        ../Common/gfx/image_file.h
		../Common/gfx/image_pcx.cpp
		../Common/gfx/image_png.cpp
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
        ../Common/util/ini_util.cpp
        ../Common/util/ini_util.h
        ../Common/util/inifile.cpp
        ../Common/util/inifile.h
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
        ../Common/util/textstreamreader.h
        ../Common/util/textstreamwriter.cpp
        ../Common/util/textstreamwriter.h
        ../Common/util/textwriter.h)


#-----------------------------------------------------------------------------#
# Minimal Allegro config
#-----------------------------------------------------------------------------#

include(TestBigEndian)

test_big_endian(ALLEGRO_BIG_ENDIAN)
if(NOT ALLEGRO_BIG_ENDIAN)
   set(ALLEGRO_LITTLE_ENDIAN 1)
endif(NOT ALLEGRO_BIG_ENDIAN)

if(CMAKE_COMPILER_IS_GNUCC)
    set(ALLEGRO_GCC 1)
endif(CMAKE_COMPILER_IS_GNUCC)

if(MSVC)
    set(ALLEGRO_MSVC 1)
endif()

#-----------------------------------------------------------------------------#

target_include_directories(libtools PUBLIC ../libsrc/allegro/include)
set(TOOLS_MINIMAL_ALLEGRO_SOURCES
        ../libsrc/allegro/src/color.c
        data/minallegro_inline.c)
target_compile_definitions(libtools PRIVATE ALLEGRO_STATICLINK)

target_sources(libtools
        PRIVATE
        data/agfreader.cpp
        data/agfreader.h
        data/data_file_writer.cpp
        data/data_file_writer.h
        data/dialogscriptconv.cpp
        data/dialogscriptconv.h
        data/game_utils.h
        data/game_utils.cpp
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
        ${TOOLS_MINIMAL_ALLEGRO_SOURCES}
        )
        
target_link_libraries(libtools PUBLIC MiniZ::MiniZ)
target_link_libraries(libtools PUBLIC stb::stb)
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

#----- agf2dta ---------------------------------------------
add_executable(agf2dta agf2dta/main.cpp)
set_target_properties(agf2dta PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
)
target_link_libraries(agf2dta PUBLIC libtools)

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
        agf2dta
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
      ${CMAKE_SOURCE_DIR}/Common/test/common_stubs.cpp # GUI reader runtime stubs used by data_file_writer_test
      ../Common/ac/gamesetupstructbase.cpp # needed by data_file_writer_test
      ../Common/ac/view.cpp # needed by data_file_writer_test
      ../Common/ac/audiocliptype.cpp # needed by data_file_writer_test
      ../Common/ac/dynobj/scriptaudioclip.cpp # needed by data_file_writer_test
      ../Common/ac/dialogtopic.cpp # needed by data_file_writer_test
      ../Common/ac/common.cpp # needed by GUI readers in data_file_writer_test
      ../Common/ac/spritecache.cpp # needed by GUI readers in data_file_writer_test
      ../Common/data/assetmanager.cpp # needed by Common font renderers
      ../Common/font/fonts.cpp # needed by GUI readers in data_file_writer_test
      ../Common/font/ttffontrenderer.cpp # needed by fonts.cpp
      ../Common/font/wfnfont.cpp # needed by fonts.cpp
      ../Common/font/wfnfontrenderer.cpp # needed by fonts.cpp
      ../Common/game/interactions.cpp # needed by data_file_writer_test
      ../Common/game/customproperties.cpp # needed by data_file_writer_test
      ../Common/gfx/allegrobitmap.cpp # needed by GUI readers in data_file_writer_test
      ../Common/gfx/bitmap.cpp # needed by allegrobitmap.cpp
      ../Common/gui/guibutton.cpp # needed by data_file_writer_test
      ../Common/gui/guiinv.cpp # needed by data_file_writer_test
      ../Common/gui/guilabel.cpp # needed by data_file_writer_test
      ../Common/gui/guilistbox.cpp # needed by data_file_writer_test
      ../Common/gui/guimain.cpp # needed by data_file_writer_test
      ../Common/gui/guiobject.cpp # needed by data_file_writer_test
      ../Common/gui/guislider.cpp # needed by data_file_writer_test
      ../Common/gui/guitextbasedcontrol.cpp # needed by data_file_writer_test
      ../Common/gui/guitextbox.cpp # needed by data_file_writer_test
      ../Common/util/geometry.cpp # needed by GUI readers in data_file_writer_test
      ../Common/util/path_ex.cpp # needed by assetmanager.cpp
      ../Common/util/version.cpp # needed by common_stubs.cpp
      ../Common/util/wgt2allg.cpp # needed by Common bitmap/font implementations
      test/agfreader_test.cpp
      test/data_file_writer_test.cpp
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
      Allegro::Allegro
      libtools
      AlFont::AlFont
      AAStr::AAStr
      gtest_main
   )

   include(GoogleTest)
   gtest_add_tests(TARGET tools_test)
endif()
