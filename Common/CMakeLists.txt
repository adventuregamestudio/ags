include(TestBigEndian)
include(CheckFseeko)

add_library(common STATIC EXCLUDE_FROM_ALL)

set_target_properties(common PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS NO
    C_STANDARD 11
    C_EXTENSIONS YES
)
set_property(TARGET common PROPERTY POSITION_INDEPENDENT_CODE ON)

target_include_directories(common PUBLIC .)

target_sources(common 
    PRIVATE
    ac/audiocliptype.cpp
    ac/audiocliptype.h
    ac/characterinfo.cpp
    ac/characterinfo.h
    ac/common.cpp
    ac/common.h
    ac/common_defines.h
    ac/dialogtopic.cpp
    ac/dialogtopic.h
    ac/dynobj/scriptaudioclip.cpp
    ac/dynobj/scriptaudioclip.h
    ac/game_version.h
    ac/gamesetupstruct.cpp
    ac/gamesetupstruct.h
    ac/gamesetupstructbase.cpp
    ac/gamesetupstructbase.h
    ac/gamestructdefines.h
    ac/interfacebutton.h
    ac/interfaceelement.h
    ac/inventoryiteminfo.cpp
    ac/inventoryiteminfo.h
    ac/keycode.cpp
    ac/keycode.h
    ac/mousecursor.cpp
    ac/mousecursor.h
    ac/oldgamesetupstruct.h
    ac/spritecache.cpp
    ac/spritecache.h
    ac/spritefile.cpp
    ac/spritefile.h
    ac/view.cpp
    ac/view.h
    ac/wordsdictionary.cpp
    ac/wordsdictionary.h
    core/asset.cpp
    core/asset.h
    core/assetmanager.cpp
    core/assetmanager.h
    core/def_version.h
    core/types.h
    debug/assert.h
    debug/debugmanager.cpp
    debug/debugmanager.h
    debug/out.h
    debug/outputhandler.h
    font/agsfontrenderer.h
    font/fonts.cpp
    font/fonts.h
    font/ttffontrenderer.cpp
    font/ttffontrenderer.h
    font/wfnfont.cpp
    font/wfnfont.h
    font/wfnfontrenderer.cpp
    font/wfnfontrenderer.h
    game/customproperties.cpp
    game/customproperties.h
    game/interactions.cpp
    game/interactions.h
    game/main_game_file.cpp
    game/main_game_file.h
    game/plugininfo.h
    game/room_file.cpp
    game/room_file.h
    game/room_file_base.cpp
    game/room_file_deprecated.cpp
    game/room_version.h
    game/roomstruct.cpp
    game/roomstruct.h
    game/tra_file.cpp
    game/tra_file.h
    gfx/allegrobitmap.cpp
    gfx/allegrobitmap.h
    gfx/bitmap.cpp
    gfx/bitmap.h
    gfx/gfx_def.h
    gui/guibutton.cpp
    gui/guibutton.h
    gui/guidefines.h
    gui/guiinv.cpp
    gui/guiinv.h
    gui/guilabel.cpp
    gui/guilabel.h
    gui/guilistbox.cpp
    gui/guilistbox.h
    gui/guimain.cpp
    gui/guimain.h
    gui/guiobject.cpp
    gui/guiobject.h
    gui/guislider.cpp
    gui/guislider.h
    gui/guitextbox.cpp
    gui/guitextbox.h
    script/cc_common.cpp
    script/cc_common.h
    script/cc_internal.h
    script/cc_script.cpp
    script/cc_script.h
    util/aasset_stream.cpp
    util/aasset_stream.h
    util/android_file.cpp
    util/android_file.h
    util/bbop.h
    util/cmdlineopts.cpp
    util/cmdlineopts.h
    util/compress.cpp
    util/compress.h
    util/data_ext.cpp
    util/data_ext.h
    util/datastream.cpp
    util/datastream.h
    util/directory.cpp
    util/directory.h
    util/error.h
    util/file.cpp
    util/file.h
    util/filestream.cpp
    util/filestream.h
    util/geometry.cpp
    util/geometry.h
    util/ini_util.cpp
    util/ini_util.h
    util/inifile.cpp
    util/inifile.h
    util/lzw.cpp
    util/lzw.h
    util/math.h
    util/memory.h
    util/memory_compat.h
    util/memorystream.cpp
    util/memorystream.h
    util/multifilelib.h
    util/multifilelib.cpp
    util/path.cpp
    util/path_ex.cpp
    util/path.h
    util/resourcecache.h
    util/scaling.h
    util/stdio_compat.c
    util/stdio_compat.h
    util/stream.cpp
    util/stream.h
    util/string.cpp
    util/string.h
    util/string_types.h
    util/string_utils.cpp
    util/string_utils.h
    util/textreader.h
    util/textstreamreader.cpp
    util/textstreamreader.h
    util/textstreamwriter.cpp
    util/textstreamwriter.h
    util/textwriter.h
    util/version.cpp
    util/version.h
    util/wgt2allg.cpp
    util/wgt2allg.h
    util/bufferedstream.cpp
    util/bufferedstream.h
    util/string_compat.c
    util/string_compat.h
    util/matrix.h

	platform/windows/windows.h
)

target_link_libraries(common PUBLIC
        ${SDL2_LIBRARY} ${SDL2_LIBRARIES}
        Allegro::Allegro
        AlFont::AlFont
        AAStr::AAStr
        glm::glm
        MiniZ::MiniZ)

if (WIN32)
    target_link_libraries(common PUBLIC shlwapi)
endif()

if(ANDROID)
    find_library(ANDROID_LIB android)
    target_link_libraries(common PUBLIC ${ANDROID_LIB})
endif()

# NOTE: You can optionally create case sensitive filesystems on Macos and Windows now.
if (LINUX OR ANDROID)
    target_compile_definitions(common PRIVATE AGS_CASE_SENSITIVE_FILESYSTEM)
endif()

if (HAVE_FSEEKO)
    target_compile_definitions(common PUBLIC HAVE_FSEEKO)
endif()

if(NOT MSVC)
    target_compile_options(common PUBLIC -Werror=undef)
endif()

if(AGS_DEBUG_MANAGED_OBJECTS)
    target_compile_definitions(common PUBLIC "DEBUG_MANAGED_OBJECTS=1")
endif()

if(AGS_DEBUG_SPRITECACHE)
    target_compile_definitions(common PUBLIC "DEBUG_SPRITECACHE=1")
endif()

get_target_property(COMMON_SOURCES common SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${COMMON_SOURCES})

add_library(AGS::Common ALIAS common)

if(AGS_TESTS)
    add_executable(common_test
        test/cmdlineopts_test.cpp
        test/gfxdef_test.cpp
        test/inifile_test.cpp
        test/math_test.cpp
        test/memory_test.cpp
        test/path_test.cpp
        test/stream_test.cpp
        test/string_test.cpp
        test/version_test.cpp
    )
    set_target_properties(common_test PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_EXTENSIONS NO
        INTERPROCEDURAL_OPTIMIZATION FALSE
        )
    target_link_libraries(common_test
        common
        gtest_main
    )

    include(GoogleTest)
    gtest_add_tests(TARGET common_test)
endif()