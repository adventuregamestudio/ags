# Engine
# -----------------------------------------------------------------------------

if (NOT ANDROID)
add_library(engine STATIC)
else ()
add_library(engine SHARED)
endif ()


set_target_properties(engine PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS NO
    C_STANDARD 11
    C_EXTENSIONS YES
)

target_include_directories(engine PUBLIC .)

target_sources(engine
    PRIVATE
    ac/asset_helper.h
    ac/audiochannel.cpp
    ac/audiochannel.h
    ac/audioclip.cpp
    ac/audioclip.h
    ac/button.cpp
    ac/button.h
    ac/character.cpp
    ac/character.h
    ac/characterextras.cpp
    ac/characterextras.h
    ac/characterinfo_engine.cpp
    ac/datetime.cpp
    ac/datetime.h
    ac/dialog.cpp
    ac/dialog.h
    ac/dialogoptionsrendering.cpp
    ac/dialogoptionsrendering.h
    ac/display.cpp
    ac/display.h
    ac/draw.cpp
    ac/draw.h
    ac/draw_software.cpp
    ac/draw_software.h
    ac/drawingsurface.cpp
    ac/drawingsurface.h
    ac/dynamicsprite.cpp
    ac/dynamicsprite.h
    ac/dynobj/all_dynamicclasses.h
    ac/dynobj/all_scriptclasses.h
    ac/dynobj/cc_agsdynamicobject.cpp
    ac/dynobj/cc_agsdynamicobject.h
    ac/dynobj/cc_audiochannel.cpp
    ac/dynobj/cc_audiochannel.h
    ac/dynobj/cc_audioclip.cpp
    ac/dynobj/cc_audioclip.h
    ac/dynobj/cc_character.cpp
    ac/dynobj/cc_character.h
    ac/dynobj/cc_dialog.cpp
    ac/dynobj/cc_dialog.h
    ac/dynobj/cc_dynamicarray.cpp
    ac/dynobj/cc_dynamicarray.h
    ac/dynobj/cc_scriptobject.h
    ac/dynobj/cc_gui.cpp
    ac/dynobj/cc_gui.h
    ac/dynobj/cc_guiobject.cpp
    ac/dynobj/cc_guiobject.h
    ac/dynobj/cc_hotspot.cpp
    ac/dynobj/cc_hotspot.h
    ac/dynobj/cc_inventory.cpp
    ac/dynobj/cc_inventory.h
    ac/dynobj/cc_object.cpp
    ac/dynobj/cc_object.h
    ac/dynobj/cc_pluginobject.h
    ac/dynobj/cc_region.cpp
    ac/dynobj/cc_region.h
    ac/dynobj/cc_serializer.cpp
    ac/dynobj/cc_serializer.h
    ac/dynobj/dynobj_manager.cpp
    ac/dynobj/dynobj_manager.h
    ac/dynobj/managedobjectpool.cpp
    ac/dynobj/managedobjectpool.h
    ac/dynobj/scriptaudiochannel.h
    ac/dynobj/scriptcamera.cpp
    ac/dynobj/scriptcamera.h
    ac/dynobj/scriptcontainers.h
    ac/dynobj/scriptdatetime.cpp
    ac/dynobj/scriptdatetime.h
    ac/dynobj/scriptdialog.h
    ac/dynobj/scriptdialogoptionsrendering.cpp
    ac/dynobj/scriptdialogoptionsrendering.h
    ac/dynobj/scriptdict.cpp
    ac/dynobj/scriptdict.h
    ac/dynobj/scriptdrawingsurface.cpp
    ac/dynobj/scriptdrawingsurface.h
    ac/dynobj/scriptdynamicsprite.cpp
    ac/dynobj/scriptdynamicsprite.h
    ac/dynobj/scriptfile.cpp
    ac/dynobj/scriptfile.h
    ac/dynobj/scriptjoystick.cpp
    ac/dynobj/scriptjoystick.h
    ac/dynobj/scriptgui.h
    ac/dynobj/scripthotspot.h
    ac/dynobj/scriptinvitem.h
    ac/dynobj/scriptmouse.cpp
    ac/dynobj/scriptmouse.h
    ac/dynobj/scriptmouse.cpp
    ac/dynobj/scriptobject.h
    ac/dynobj/scriptoverlay.cpp
    ac/dynobj/scriptoverlay.h
    ac/dynobj/scriptregion.h
    ac/dynobj/scriptset.cpp
    ac/dynobj/scriptset.h
    ac/dynobj/scriptstring.cpp
    ac/dynobj/scriptstring.h
    ac/dynobj/scriptsystem.cpp
    ac/dynobj/scriptsystem.h
    ac/dynobj/scriptsystem.cpp
    ac/dynobj/scriptuserobject.cpp
    ac/dynobj/scriptuserobject.h
    ac/dynobj/scriptviewframe.cpp
    ac/dynobj/scriptviewframe.h
    ac/dynobj/scriptviewport.cpp
    ac/dynobj/scriptviewport.h
    ac/event.cpp
    ac/event.h
    ac/file.cpp
    ac/file.h
    ac/game.cpp
    ac/game.h
    ac/joystick.cpp
    ac/joystick.h
    ac/gamesetup.cpp
    ac/gamesetup.h
    ac/gamestate.cpp
    ac/gamestate.h
    ac/global_api.cpp
    ac/global_audio.cpp
    ac/global_audio.h
    ac/global_character.cpp
    ac/global_character.h
    ac/global_debug.cpp
    ac/global_debug.h
    ac/global_dialog.cpp
    ac/global_dialog.h
    ac/global_display.cpp
    ac/global_display.h
    ac/global_dynamicsprite.cpp
    ac/global_dynamicsprite.h
    ac/global_file.cpp
    ac/global_file.h
    ac/global_game.cpp
    ac/global_game.h
    ac/global_gui.cpp
    ac/global_gui.h
    ac/global_hotspot.cpp
    ac/global_hotspot.h
    ac/global_inventoryitem.cpp
    ac/global_inventoryitem.h
    ac/global_object.cpp
    ac/global_object.h
    ac/global_palette.cpp
    ac/global_palette.h
    ac/global_region.cpp
    ac/global_region.h
    ac/global_room.cpp
    ac/global_room.h
    ac/global_screen.cpp
    ac/global_screen.h
    ac/global_timer.cpp
    ac/global_timer.h
    ac/global_translation.cpp
    ac/global_translation.h
    ac/global_video.cpp
    ac/global_video.h
    ac/global_walkablearea.cpp
    ac/global_walkablearea.h
    ac/global_walkbehind.cpp
    ac/global_walkbehind.h
    ac/gui.cpp
    ac/gui.h
    ac/guicontrol.cpp
    ac/guicontrol.h
    ac/guiinv.cpp
    ac/hotspot.cpp
    ac/hotspot.h
    ac/interfacebutton.cpp
    ac/interfaceelement.cpp
    ac/inventoryitem.cpp
    ac/inventoryitem.h
    ac/invwindow.cpp
    ac/invwindow.h
    ac/label.cpp
    ac/label.h
    ac/lipsync.h
    ac/listbox.cpp
    ac/listbox.h
    ac/math.cpp
    ac/math.h
    ac/mouse.cpp
    ac/mouse.h
    ac/movelist.cpp
    ac/movelist.h
    ac/object.cpp
    ac/object.h
    ac/overlay.cpp
    ac/overlay.h
    ac/parser.cpp
    ac/parser.h
    ac/path_helper.h
    ac/properties.cpp
    ac/properties.h
    ac/region.cpp
    ac/region.h
    ac/room.cpp
    ac/room.h
    ac/roomobject.cpp
    ac/roomobject.h
    ac/roomstatus.cpp
    ac/roomstatus.h
    ac/route_finder.cpp
    ac/route_finder.h
    ac/route_finder_impl.cpp
    ac/route_finder_impl.h
    ac/route_finder_jps.inl
    ac/runtime_defines.h
    ac/screen.cpp
    ac/screen.h
    ac/screenoverlay.cpp
    ac/screenoverlay.h
    ac/scriptcontainers.cpp
    ac/slider.cpp
    ac/slider.h
    ac/speech.cpp
    ac/speech.h
    ac/sprite.cpp
    ac/sprite.h
    ac/dynobj/scriptgame.cpp
    ac/dynobj/scriptgame.h
    ac/dynobj/cc_staticarray.cpp
    ac/dynobj/cc_staticarray.h
    ac/string.cpp
    ac/string.h
    ac/sys_events.cpp
    ac/sys_events.h
    ac/system.cpp
    ac/system.h
    ac/textbox.cpp
    ac/textbox.h
    ac/timer.cpp
    ac/timer.h
    ac/topbarsettings.h
    ac/translation.cpp
    ac/translation.h
    ac/viewframe.cpp
    ac/viewframe.h
    ac/viewport_script.cpp
    ac/walkablearea.cpp
    ac/walkablearea.h
    ac/walkbehind.cpp
    ac/walkbehind.h
    debug/agseditordebugger.h
    debug/debug.cpp
    debug/debug_log.h
    debug/debugger.h
    debug/dummyagsdebugger.h
    debug/filebasedagsdebugger.cpp
    debug/filebasedagsdebugger.h
    debug/logfile.cpp
    debug/logfile.h
    debug/messagebuffer.cpp
    debug/messagebuffer.h
    device/mousew32.cpp
    device/mousew32.h
    font/fonts_engine.cpp
    game/game_init.cpp
    game/game_init.h
    game/savegame.cpp
    game/savegame.h
    game/savegame_components.cpp
    game/savegame_components.h
    game/savegame_internal.h
    game/viewport.cpp
    game/viewport.h
    gfx/ali3dexception.h
    gfx/ali3dogl.cpp
    gfx/ali3dogl.h
    gfx/ali3dsw.cpp
    gfx/ali3dsw.h
    gfx/blender.cpp
    gfx/blender.h
    gfx/color_engine.cpp
    gfx/ddb.h
    gfx/gfx_util.cpp
    gfx/gfx_util.h
    gfx/gfxdefines.h
    gfx/gfxdriverbase.cpp
    gfx/gfxdriverbase.h
    gfx/gfxdriverfactory.cpp
    gfx/gfxdriverfactory.h
    gfx/gfxdriverfactorybase.h
    gfx/gfxfilter.h
    gfx/gfxfilter_aad3d.cpp
    gfx/gfxfilter_aad3d.h
    gfx/gfxfilter_aaogl.cpp
    gfx/gfxfilter_aaogl.h
    gfx/gfxfilter_d3d.cpp
    gfx/gfxfilter_d3d.h
    gfx/gfxfilter_ogl.cpp
    gfx/gfxfilter_ogl.h
    gfx/gfxfilter_scaling.cpp
    gfx/gfxfilter_scaling.h
    gfx/gfxfilter_sdl_renderer.cpp
    gfx/gfxfilter_sdl_renderer.h
    gfx/gfxmodelist.h
    gfx/graphicsdriver.h
    gfx/ogl_headers.h
    gui/animatingguibutton.cpp
    gui/animatingguibutton.h
    gui/cscidialog.cpp
    gui/cscidialog.h
    gui/gui_engine.cpp
    gui/guidialog.cpp
    gui/guidialog.h
    gui/guidialogdefines.h
    gui/guidialoginternaldefs.h
    gui/mycontrols.h
    gui/mylabel.cpp
    gui/mylabel.h
    gui/mylistbox.cpp
    gui/mylistbox.h
    gui/mypushbutton.cpp
    gui/mypushbutton.h
    gui/mytextbox.cpp
    gui/mytextbox.h
    gui/newcontrol.cpp
    gui/newcontrol.h
    main/config.cpp
    main/config.h
    main/engine.cpp
    main/engine.h
    main/engine_setup.cpp
    main/engine_setup.h
    main/game_file.cpp
    main/game_file.h
    main/game_run.cpp
    main/game_run.h
    main/game_start.cpp
    main/game_start.h
    main/graphics_mode.cpp
    main/graphics_mode.h
    main/main.cpp
    main/main.h
    main/quit.cpp
    main/quit.h
    main/update.cpp
    main/update.h
    media/audio/audio_core.cpp
    media/audio/audio_core.h
    media/audio/audio.cpp
    media/audio/audio.h
    media/audio/audio_system.h
    media/audio/audiodefines.h
    media/audio/sdldecoder.cpp
    media/audio/sdldecoder.h
    media/audio/openalsource.cpp
    media/audio/openalsource.h
    media/audio/openal.h
    media/audio/queuedaudioitem.cpp
    media/audio/queuedaudioitem.h
    media/audio/sound.cpp
    media/audio/sound.h
    media/audio/soundclip.cpp
    media/audio/soundclip.h
    media/video/video.cpp
    media/video/video.h
    platform/base/agsplatformdriver.cpp
    platform/base/agsplatformdriver.h
    platform/base/agsplatform_xdg_unix.cpp
    platform/base/agsplatform_xdg_unix.h
    platform/base/sys_main.cpp
    platform/base/sys_main.h
    platform/util/libc.c
    plugin/agsplugin.cpp
    plugin/agsplugin.h
    plugin/agsplugin_evts.h
    plugin/plugin_builtin.h
    plugin/plugin_engine.h
    plugin/plugin_stubs.cpp
    resource/resource.h
    script/cc_instance.cpp
    script/cc_instance.h
    script/cc_reflecthelper.cpp
    script/cc_reflecthelper.h
    script/executingscript.cpp
    script/executingscript.h
    script/exports.cpp
    script/exports.h
    script/nonblockingscriptfunction.h
    script/runtimescriptvalue.cpp
    script/runtimescriptvalue.h
    script/script.cpp
    script/script.h
    script/script_api.cpp
    script/script_api.h
    script/script_runtime.cpp
    script/script_runtime.h
    script/systemimports.cpp
    script/systemimports.h
    util/library.h
    util/library_dummy.h
    util/library_posix.h
    util/sdl2_util.h
    util/sdl2_util.cpp

    platform/windows/acplwin.cpp
    platform/windows/debug/namedpipesagsdebugger.cpp
    platform/windows/gfx/ali3dd3d.cpp
    platform/windows/setup/winsetup.cpp
    platform/windows/minidump.cpp
    platform/windows/win_ex_handling.cpp

    platform/linux/acpllnx.cpp

    platform/osx/acplmac.cpp

    platform/android/acpland.cpp

    platform/ios/acplios.cpp

    platform/emscripten/acpemscripten.cpp

    platform/bsd/acplbsd.cpp

    platform/windows/win_ex_handling.cpp
    platform/windows/win_ex_handling.h
    platform/windows/minidump.cpp
    platform/windows/winapi_exclusive.h

    platform/base/mobile_base.cpp
    platform/base/mobile_base.h
)

if(ANDROID)
    target_sources(engine PRIVATE
        platform/android/helper/jni_helper.cpp)
endif()

if(MACOS)
    target_sources(engine PRIVATE 
        platform/osx/alplmac.mm
    )
endif()

if (AGS_BUILTIN_PLUGINS)
    target_compile_definitions(engine PRIVATE BUILTIN_PLUGINS)

    target_sources(engine PRIVATE 
        ../Plugins/agsblend/AGSBlend.cpp
        ../Plugins/agsblend/agsblend.h

        ../Plugins/AGSflashlight/agsflashlight.cpp
        ../Plugins/AGSflashlight/agsflashlight.h

        ../Plugins/ags_parallax/ags_parallax.cpp
        ../Plugins/ags_parallax/ags_parallax.h

        ../Plugins/ags_snowrain/ags_snowrain.cpp
        ../Plugins/ags_snowrain/ags_snowrain.h

        ../Plugins/AGSSpriteFont/AGSSpriteFont/AGSSpriteFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/AGSSpriteFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/CharacterEntry.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/CharacterEntry.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRenderer.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRenderer.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRendererClifftopGames.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRendererClifftopGames.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFontClifftopGames.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFontClifftopGames.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/color.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/color.h

        ../Plugins/agstouch/agstouch.cpp
        ../Plugins/agstouch/agstouch.h

        ../Plugins/agspalrender/palrender.h
        ../Plugins/agspalrender/ags_palrender.cpp
        ../Plugins/agspalrender/raycast.cpp
        ../Plugins/agspalrender/raycast.h
    )
endif()

if (AGS_BUILD_STR)
    target_compile_definitions(engine PUBLIC BUILD_STR=\"${AGS_BUILD_STR}\")
endif()


target_link_libraries(engine PUBLIC 
    AGS::Common 
    ${CMAKE_DL_LIBS} 
    Allegro::Allegro
    External::OpenAL
    ${SDL2_LIBRARY}
    ${SDL2_LIBRARIES}
    SDL2_sound::SDL2_sound
)

if(NOT AGS_DISABLE_THREADS)
    target_link_libraries(engine PUBLIC Threads::Threads)
endif()

if (AGS_OPENGLES2)
    target_link_libraries(engine PUBLIC EGL GLESv2 Glad::GladGLES2)
else()
    target_link_libraries(engine PUBLIC Glad::Glad)
endif (AGS_OPENGLES2)

# common has a circular dependency on engine
# TODO: currently required for gcc support, will look into this.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
target_link_libraries(common PUBLIC engine)
endif ()

if (WIN32 OR LINUX)
    target_compile_definitions(engine PRIVATE AGS_HAS_CD_AUDIO)
endif ()

if (AGS_NO_VIDEO_PLAYER)
    target_compile_definitions(engine PRIVATE AGS_NO_VIDEO_PLAYER)
else()
    target_link_libraries(engine PUBLIC Apeg::Apeg)
    if (WIN32)
        target_link_libraries(engine PUBLIC amstrmid quartz)
    endif()
endif()

if (WIN32)
    target_link_libraries(engine PUBLIC shlwapi)
endif()

if(ANDROID)
    find_library(ANDROID_LOG_LIB log)
    target_link_libraries(engine PUBLIC ${ANDROID_LOG_LIB})
endif()

get_target_property(ENGINE_SOURCES engine SOURCES)
# Set ROOT to parent directory to include Plugins/
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. PREFIX "Source Files" FILES ${ENGINE_SOURCES})

# Standalone exe
# -----------------------------------------------------------------------------
if (NOT ANDROID)
    add_executable(ags)
else ()
    add_library(ags SHARED)
endif (NOT ANDROID)

if (AGS_DESKTOP)
    install(TARGETS ags RUNTIME DESTINATION bin)
endif ()

set_target_properties(ags PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS NO
)

target_include_directories(ags PRIVATE .)

target_sources(ags
    PRIVATE
    main/main_sdl2.cpp
)

target_link_libraries(ags PRIVATE engine ${SDL2MAIN_LIBRARY})

if (LINUX)
    # We may override `allegro_icon` and `load_midi_pf` depending on the version
    target_link_options(ags PRIVATE -Wl,--allow-multiple-definition)
endif ()

set_target_properties(ags PROPERTIES
    RUNTIME_OUTPUT_NAME ags
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if (WIN32)
    target_sources(ags PRIVATE resource/version.rc) # for setup dialog and d3d shaders
    set_target_properties(ags PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# Test
# -----------------------------------------------------------------------------
if(AGS_TESTS)
    add_executable(
        engine_test
        test/scsprintf_test.cpp
    )
    set_target_properties(engine_test PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
        C_STANDARD 11
        C_EXTENSIONS NO
        INTERPROCEDURAL_OPTIMIZATION FALSE
        )
    target_link_libraries(
        engine_test
        engine
        gtest_main
    )

    include(GoogleTest)
    gtest_add_tests(TARGET engine_test)
endif()

# macOS App Bundle
# -----------------------------------------------------------------------------

if (MACOS)
    add_executable(ags-app)

    set_target_properties(ags-app PROPERTIES
        CXX_STANDARD 11
        CXX_EXTENSIONS NO
    )

    target_include_directories(ags-app PRIVATE .)

    file(GLOB RESOURCE_FILES
        CONFIGURE_DEPENDS
        ../OSX/Resources/ags.icns
        ../OSX/Resources/*.ags
        ../OSX/Resources/acsetup.cfg
        ../OSX/Resources/audio.vox
        ../OSX/Resources/music.vox
        ../OSX/Resources/speech.vox
        ../OSX/Resources/*.vox
    )

    target_sources(ags-app
        PRIVATE
        main/main_sdl2.cpp
        ${RESOURCE_FILES}
    )

    target_link_libraries(ags-app PRIVATE engine SDL2::SDL2main)

    set_target_properties(ags-app PROPERTIES
        RUNTIME_OUTPUT_NAME AGS
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        MACOSX_BUNDLE TRUE
        RESOURCE "${RESOURCE_FILES}"  # *must* be part of target sources too

        # Below properties will configure the default template MacOSXBundleInfo.plist.in
        # Refer to https://cmake.org/cmake/help/latest/prop_tgt/MACOSX_BUNDLE_INFO_PLIST.html
        # if you want to manage a custom plist template file.

        MACOSX_BUNDLE_GUI_IDENTIFIER        # CFBundleIdentifier
            uk.co.adventuregamestudio.ags
        MACOSX_BUNDLE_BUNDLE_NAME           # CFBundleName
            AGS
        MACOSX_BUNDLE_ICON_FILE             # CFBundleIconFile
            ags.icns
        MACOSX_BUNDLE_BUNDLE_VERSION        # CFBundleVersion
            1.0.0
        MACOSX_BUNDLE_LONG_VERSION_STRING   # CFBundleLongVersionString
            1.0.0
        MACOSX_BUNDLE_SHORT_VERSION_STRING  # CFBundleShortVersionString
            1.0.0
        MACOSX_BUNDLE_COPYRIGHT             # NSHumanReadableCopyright
            "Copyright (C) 1999-2011 Chris Jones and 2011-20xx others"
        MACOSX_BUNDLE_INFO_STRING           # CFBundleGetInfoString
            "Created by Adventure Game Studio"
    )
endif()
