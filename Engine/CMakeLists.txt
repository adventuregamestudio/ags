include(CheckLoadMidiPf)


# Engine
# -----------------------------------------------------------------------------

add_library(engine)

set_target_properties(engine PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS NO
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
    ac/cdaudio.cpp
    ac/cdaudio.h
    ac/character.cpp
    ac/character.h
    ac/charactercache.h
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
    ac/dynobj/cc_dynamicobject.cpp
    ac/dynobj/cc_dynamicobject.h
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
    ac/dynobj/cc_region.cpp
    ac/dynobj/cc_region.h
    ac/dynobj/cc_serializer.cpp
    ac/dynobj/cc_serializer.h
    ac/dynobj/managedobjectpool.cpp
    ac/dynobj/managedobjectpool.h
    ac/dynobj/scriptaudiochannel.h
    ac/dynobj/scriptcamera.cpp
    ac/dynobj/scriptcamera.h
    ac/dynobj/scriptdatetime.cpp
    ac/dynobj/scriptdatetime.h
    ac/dynobj/scriptdialog.h
    ac/dynobj/scriptdialogoptionsrendering.cpp
    ac/dynobj/scriptdialogoptionsrendering.h
    ac/dynobj/scriptdrawingsurface.cpp
    ac/dynobj/scriptdrawingsurface.h
    ac/dynobj/scriptdynamicsprite.cpp
    ac/dynobj/scriptdynamicsprite.h
    ac/dynobj/scriptfile.cpp
    ac/dynobj/scriptfile.h
    ac/dynobj/scriptgui.h
    ac/dynobj/scripthotspot.h
    ac/dynobj/scriptinvitem.h
    ac/dynobj/scriptmouse.h
    ac/dynobj/scriptobject.h
    ac/dynobj/scriptoverlay.cpp
    ac/dynobj/scriptoverlay.h
    ac/dynobj/scriptregion.h
    ac/dynobj/scriptstring.cpp
    ac/dynobj/scriptstring.h
    ac/dynobj/scriptsystem.h
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
    ac/gamesetup.cpp
    ac/gamesetup.h
    ac/gamestate.cpp
    ac/gamestate.h
    ac/global_api.cpp
    ac/global_audio.cpp
    ac/global_audio.h
    ac/global_button.cpp
    ac/global_button.h
    ac/global_character.cpp
    ac/global_character.h
    ac/global_datetime.cpp
    ac/global_datetime.h
    ac/global_debug.cpp
    ac/global_debug.h
    ac/global_dialog.cpp
    ac/global_dialog.h
    ac/global_display.cpp
    ac/global_display.h
    ac/global_drawingsurface.cpp
    ac/global_drawingsurface.h
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
    ac/global_invwindow.cpp
    ac/global_invwindow.h
    ac/global_label.cpp
    ac/global_label.h
    ac/global_listbox.cpp
    ac/global_listbox.h
    ac/global_mouse.cpp
    ac/global_mouse.h
    ac/global_object.cpp
    ac/global_object.h
    ac/global_overlay.cpp
    ac/global_overlay.h
    ac/global_palette.cpp
    ac/global_palette.h
    ac/global_parser.cpp
    ac/global_parser.h
    ac/global_plugin.h
    ac/global_record.cpp
    ac/global_record.h
    ac/global_region.cpp
    ac/global_region.h
    ac/global_room.cpp
    ac/global_room.h
    ac/global_screen.cpp
    ac/global_screen.h
    ac/global_slider.cpp
    ac/global_slider.h
    ac/global_string.cpp
    ac/global_string.h
    ac/global_textbox.cpp
    ac/global_textbox.h
    ac/global_timer.cpp
    ac/global_timer.h
    ac/global_translation.cpp
    ac/global_translation.h
    ac/global_video.cpp
    ac/global_video.h
    ac/global_viewframe.cpp
    ac/global_viewframe.h
    ac/global_viewport.cpp
    ac/global_viewport.h
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
    ac/keycode.h
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
    ac/objectcache.h
    ac/overlay.cpp
    ac/overlay.h
    ac/parser.cpp
    ac/parser.h
    ac/path_helper.h
    ac/properties.cpp
    ac/properties.h
    ac/region.cpp
    ac/region.h
    ac/richgamemedia.cpp
    ac/richgamemedia.h
    ac/room.cpp
    ac/room.h
    ac/roomobject.cpp
    ac/roomobject.h
    ac/roomstatus.cpp
    ac/roomstatus.h
    ac/route_finder.cpp
    ac/route_finder.h
    ac/runtime_defines.h
    ac/screen.cpp
    ac/screen.h
    ac/screenoverlay.cpp
    ac/screenoverlay.h
    ac/slider.cpp
    ac/slider.h
    ac/speech.cpp
    ac/speech.h
    ac/sprite.cpp
    ac/sprite.h
    ac/spritecache_engine.cpp
    ac/spritelistentry.h
    ac/statobj/agsstaticobject.cpp
    ac/statobj/agsstaticobject.h
    ac/statobj/staticarray.cpp
    ac/statobj/staticarray.h
    ac/statobj/staticobject.h
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
    ac/tree_map.cpp
    ac/tree_map.h
    ac/viewframe.cpp
    ac/viewframe.h
    ac/viewport.cpp
    ac/walkablearea.cpp
    ac/walkablearea.h
    ac/walkbehind.cpp
    ac/walkbehind.h
    debug/agseditordebugger.h
    debug/consoleoutputtarget.cpp
    debug/consoleoutputtarget.h
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
    gfx/gfxfilter_allegro.cpp
    gfx/gfxfilter_allegro.h
    gfx/gfxfilter_d3d.cpp
    gfx/gfxfilter_d3d.h
    gfx/gfxfilter_hqx.cpp
    gfx/gfxfilter_hqx.h
    gfx/gfxfilter_ogl.cpp
    gfx/gfxfilter_ogl.h
    gfx/gfxfilter_scaling.cpp
    gfx/gfxfilter_scaling.h
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
    main/main.h
    main/main_allegro.h
    main/maindefines_ex.h
    main/mainheader.h
    main/minidump.cpp
    main/quit.cpp
    main/quit.h
    main/update.cpp
    main/update.h
    media/audio/ambientsound.cpp
    media/audio/ambientsound.h
    media/audio/audio.cpp
    media/audio/audio.h
    media/audio/audio_system.h
    media/audio/audiodefines.h
    media/audio/audiointernaldefs.h
    media/audio/clip_mydumbmod.cpp
    media/audio/clip_mydumbmod.h
    media/audio/clip_myjgmod.cpp
    media/audio/clip_myjgmod.h
    media/audio/clip_mymidi.cpp
    media/audio/clip_mymidi.h
    media/audio/clip_mymp3.cpp
    media/audio/clip_mymp3.h
    media/audio/clip_myogg.cpp
    media/audio/clip_myogg.h
    media/audio/clip_mystaticmp3.cpp
    media/audio/clip_mystaticmp3.h
    media/audio/clip_mystaticogg.cpp
    media/audio/clip_mystaticogg.h
    media/audio/clip_mywave.cpp
    media/audio/clip_mywave.h
    media/audio/queuedaudioitem.cpp
    media/audio/queuedaudioitem.h
    media/audio/sound.cpp
    media/audio/sound.h
    media/audio/soundcache.cpp
    media/audio/soundcache.h
    media/audio/soundclip.cpp
    media/audio/soundclip.h
    media/video/VMR9Graph.h
    media/video/video.cpp
    media/video/video.h
    platform/base/agsplatformdriver.cpp
    platform/base/agsplatformdriver.h
    platform/base/override_defines.h
    platform/util/libc.c
    platform/util/pe.c
    platform/util/pe.h
    plugin/agsplugin.cpp
    plugin/agsplugin.h
    plugin/global_plugin.cpp
    plugin/plugin_builtin.h
    plugin/plugin_engine.h
    plugin/pluginobjectreader.cpp
    plugin/pluginobjectreader.h
    resource/resource.h
    script/cc_error_engine.cpp
    script/cc_instance.cpp
    script/cc_instance.h
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
    script/script_engine.cpp
    script/script_runtime.cpp
    script/script_runtime.h
    script/systemimports.cpp
    script/systemimports.h
    test/test_all.cpp
    test/test_all.h
    test/test_file.cpp
    test/test_gfx.cpp
    test/test_inifile.cpp
    test/test_math.cpp
    test/test_memory.cpp
    test/test_sprintf.cpp
    test/test_string.cpp
    test/test_version.cpp
    util/library.h
    util/library_dummy.h
    util/library_posix.h
    util/mutex.h
    util/mutex_base.h
    util/mutex_lock.h
    util/mutex_pthread.h
    util/mutex_std.h
    util/scaling.h
    util/thread.h
    util/thread_pthread.h
    util/thread_std.h
)

if(WIN32)
    target_sources(engine PRIVATE 
        platform/windows/acplwin.cpp
        platform/windows/debug/namedpipesagsdebugger.cpp
        platform/windows/debug/namedpipesagsdebugger.h
        platform/windows/gfx/ali3dd3d.cpp
        platform/windows/gfx/ali3dd3d.h
        platform/windows/media/video/acwavi.cpp
        platform/windows/media/video/acwavi3d.cpp
        platform/windows/setup/winsetup.cpp
        platform/windows/setup/winsetup.h
        util/mutex_windows.h
        util/library_windows.h
        util/thread_windows.h
    )
elseif(MACOS)
    #file(GLOB_RECURSE resources .game_files/*)
    target_sources(engine PRIVATE 
        platform/osx/acplmac.cpp
        platform/osx/alplmac.mm
        #${resources}
    )
elseif(LINUX)
    target_sources(engine PRIVATE 
        platform/linux/acpllnx.cpp
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
        ../Plugins/AGSSpriteFont/AGSSpriteFont/CharacterEntry.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/CharacterEntry.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRenderer.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/SpriteFontRenderer.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthFont.h
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFont.cpp
        ../Plugins/AGSSpriteFont/AGSSpriteFont/VariableWidthSpriteFont.h
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

if (NOT HAVE_LOAD_MIDI_PF)
    target_sources(engine PRIVATE libsrc/allegro-4.2.2-agspatch/midi.c)
endif()


target_compile_definitions(engine PRIVATE 
    $<$<CONFIG:DEBUG>:SOUND_CACHE_DEBUG>
    $<$<CONFIG:DEBUG>:DEBUG_MANAGED_OBJECTS>
)

if (AGS_BUILD_STR)
    AGS_target_compile_definitions(engine PUBLIC BUILD_STR=\"${AGS_BUILD_STR}\")
endif()


target_link_libraries(engine PUBLIC 
    AGS::Common 
    ${CMAKE_DL_LIBS} 
    Glad::Glad 
    Threads::Threads 
    Allegro::Allegro 
    AlDumb::AlDumb 
    AlMp3::AlMp3 
    Apeg::Apeg 
    AlOgg::AlOgg 
    HQ2X::hq2x
)

if (WIN32 OR LINUX)
    target_compile_definitions(engine PRIVATE AGS_HAS_CD_AUDIO)
    target_link_libraries(engine PUBLIC Cda::Cda)
endif ()

if (WIN32)
    target_link_libraries(engine PUBLIC DirectX::Direct3D9 amstrmid quartz shlwapi winmm)
elseif (LINUX)
    target_link_libraries(engine PUBLIC X11::X11)
endif()

get_target_property(ENGINE_SOURCES engine SOURCES)
# Set ROOT to parent directory to include Plugins/
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/.. PREFIX "Source Files" FILES ${ENGINE_SOURCES})


# Standalone exe
# -----------------------------------------------------------------------------

add_executable(ags)

set_target_properties(ags PROPERTIES
    CXX_STANDARD 11
    CXX_EXTENSIONS NO
)

target_include_directories(ags PRIVATE .)

target_sources(ags
    PRIVATE
    main/main.cpp
    main/main.h
)

if (LINUX)
    # common and engine have circular dependencies, so tell the linker to link as a group
    target_link_libraries(ags PRIVATE -Wl,--start-group common engine -Wl,--end-group)
else ()
    target_link_libraries(ags PRIVATE engine)
endif ()

set_target_properties(ags PROPERTIES
    RUNTIME_OUTPUT_NAME ags
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if (WIN32)
    set_target_properties(ags PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
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

    target_sources(ags-app
        PRIVATE
        main/main.cpp
        main/main.h
    )

    target_link_libraries(ags-app PRIVATE engine)

    set_target_properties(ags-app PROPERTIES
        RUNTIME_OUTPUT_NAME AGS
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER uk.co.adventuregamestudio.AGS
        #RESOURCE "${resources}"  # *must* be part of target sources too
        #SET_SOURCE_FILES_PROPERTIES(${Textures} PROPERTIES MACOSX_PACKAGE_LOCATION Resources) 
        #use MACOSX_PACKAGE_LOCATION for nested
    )
endif()
