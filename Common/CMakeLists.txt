include(TestBigEndian)
include(CheckFseeko)

add_library(common)

set_target_properties(common PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS NO
)

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
    ac/mousecursor.cpp
    ac/mousecursor.h
    ac/oldgamesetupstruct.h
    ac/spritecache.cpp
    ac/spritecache.h
    ac/view.cpp
    ac/view.h
    ac/wordsdictionary.cpp
    ac/wordsdictionary.h
    api/stream_api.h
    core/asset.cpp
    core/asset.h
    core/assetmanager.cpp
    core/assetmanager.h
    core/def_version.h
    core/endianness.h
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
    game/room_file_deprecated.cpp
    game/room_version.h
    game/roomstruct.cpp
    game/roomstruct.h
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
    script/cc_error.cpp
    script/cc_error.h
    script/cc_options.cpp
    script/cc_options.h
    script/cc_script.cpp
    script/cc_script.h
    script/script_common.h
    util/alignedstream.cpp
    util/alignedstream.h
    util/bbop.h
    util/c99_snprintf.h
    util/compress.cpp
    util/compress.h
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
    util/misc.cpp
    util/misc.h
    util/multifilelib.h
    util/mutifilelib.cpp
    util/path.cpp
    util/path.h
    util/proxystream.cpp
    util/proxystream.h
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
)

target_link_libraries(common PUBLIC Allegro::Allegro AlFont::AlFont AAStr::AAStr)

if(WIN32)
    target_compile_definitions(common PUBLIC WINDOWS_VERSION)
elseif(MACOS)
    target_compile_definitions(common PUBLIC MAC_VERSION)
elseif(LINUX)
    target_compile_definitions(common PUBLIC LINUX_VERSION)
else()
    message(FATAL_ERROR "Unsupported platform.")
endif()

test_big_endian(AGS_BIG_ENDIAN)
if(AGS_BIG_ENDIAN)
    target_compile_definitions(common PUBLIC AGS_BIG_ENDIAN)
else()
    target_compile_definitions(common PUBLIC AGS_LITTLE_ENDIAN)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_compile_definitions(common PUBLIC AGS_64BIT)
endif()

# NOTE: You can optionally create case sensitive filesystems on Macos and Windows now.
if (LINUX)
    target_compile_definitions(common PRIVATE AGS_CASE_SENSITIVE_FILESYSTEM)
endif()

if (HAVE_FSEEKO)
    target_compile_definitions(common PUBLIC HAVE_FSEEKO)
endif()

if (AGS_NO_MP3_PLAYER)
    target_compile_definitions(common PUBLIC NO_MP3_PLAYER)
endif()

target_compile_definitions(common PUBLIC "$<$<CONFIG:DEBUG>:DEBUG_SPRITECACHE>")

get_target_property(COMMON_SOURCES common SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${COMMON_SOURCES})

add_library(AGS::Common ALIAS common)
