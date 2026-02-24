# EditorImGui - Cross-platform AGS Editor using Dear ImGui
# Requires SDL2 + SDL_Renderer backend
# Works on Linux, Windows, macOS
# -----------------------------------------------------------------------------

add_executable(editor)

set_target_properties(editor PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS NO
    C_STANDARD 11
    C_EXTENSIONS NO
)

# Dear ImGui sources
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/backends/imgui_impl_sdlrenderer2.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

# ImGuiFileDialog
set(IMGUI_FILE_DIALOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui_file_dialog)
set(IMGUI_FILE_DIALOG_SOURCES
    ${IMGUI_FILE_DIALOG_DIR}/ImGuiFileDialog.cpp
)

# ImGuiColorTextEdit
set(IMGUI_COLOR_TEXT_EDIT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui_color_text_edit)
set(IMGUI_COLOR_TEXT_EDIT_SOURCES
    ${IMGUI_COLOR_TEXT_EDIT_DIR}/TextEditor.cpp
)

# Editor application sources
set(EDITOR_SOURCES
    src/main.cpp
    src/app.cpp
    src/app.h
    src/project/project.cpp
    src/project/project.h
    src/project/game_data.cpp
    src/project/game_data.h
    src/project/old_game_importer.cpp
    src/project/old_game_importer.h
    src/ui/editor_ui.cpp
    src/ui/editor_ui.h
    src/ui/menu_bar.cpp
    src/ui/menu_bar.h
    src/ui/project_panel.cpp
    src/ui/project_panel.h
    src/ui/properties_panel.cpp
    src/ui/properties_panel.h
    src/ui/log_panel.cpp
    src/ui/log_panel.h
    src/ui/panes/script_editor.cpp
    src/ui/panes/script_editor.h
    src/ui/panes/room_editor.cpp
    src/ui/panes/room_editor.h
    src/ui/panes/sprite_manager.cpp
    src/ui/panes/sprite_manager.h
    src/ui/panes/settings_pane.cpp
    src/ui/panes/settings_pane.h
    src/ui/panes/welcome_pane.cpp
    src/ui/panes/welcome_pane.h
    src/ui/panes/character_editor.cpp
    src/ui/panes/character_editor.h
    src/ui/panes/dialog_editor.cpp
    src/ui/panes/dialog_editor.h
    src/ui/panes/view_editor.cpp
    src/ui/panes/view_editor.h
    src/view/view_renderer.cpp
    src/view/view_renderer.h
    src/ui/panes/gui_editor.cpp
    src/ui/panes/gui_editor.h
    src/ui/panes/font_editor.cpp
    src/ui/panes/font_editor.h
    src/font/ttf_renderer.cpp
    src/font/ttf_renderer.h
    src/ui/panes/audio_manager.cpp
    src/ui/panes/audio_manager.h
    src/ui/panes/inventory_editor.cpp
    src/ui/panes/inventory_editor.h
    src/ui/panes/cursor_editor.cpp
    src/ui/panes/cursor_editor.h
    src/ui/panes/palette_editor.cpp
    src/ui/panes/palette_editor.h
    src/compiler/compiler_bridge.cpp
    src/compiler/compiler_bridge.h
    src/project/sprite_loader.cpp
    src/project/sprite_loader.h
    src/project/room_loader.cpp
    src/project/room_loader.h
    src/project/script_api_data.cpp
    src/project/script_api_data.h
    src/project/template_manager.cpp
    src/project/template_manager.h
    src/project/texture_cache.cpp
    src/project/texture_cache.h
    src/ui/file_dialog.cpp
    src/ui/file_dialog.h
    src/ui/sprite_chooser.cpp
    src/ui/sprite_chooser.h
    src/ui/dialogs/new_game_dialog.cpp
    src/ui/dialogs/new_game_dialog.h
    src/ui/dialogs/import_game_dialog.cpp
    src/ui/dialogs/import_game_dialog.h
    src/ui/panes/build_pane.cpp
    src/ui/panes/build_pane.h
    src/pipeline/build_system.cpp
    src/pipeline/build_system.h
    src/pipeline/debug_controller.cpp
    src/pipeline/debug_controller.h
    src/pipeline/speech_tools.cpp
    src/pipeline/speech_tools.h
    src/pipeline/game_data_writer.cpp
    src/pipeline/game_data_writer.h
    src/core/undo_manager.h
    src/core/shortcut_manager.h
    src/core/clipboard_manager.h
    src/core/preferences.h
    src/core/path_utils.h
    src/core/audio_player.cpp
    src/core/audio_player.h
    src/core/ogg_vorbis_impl.c
    src/ui/panes/preferences_pane.cpp
    src/ui/panes/preferences_pane.h
    src/ui/panes/translation_editor.cpp
    src/ui/panes/translation_editor.h
    src/ui/panes/global_variables_editor.cpp
    src/ui/panes/global_variables_editor.h
    src/ui/panes/custom_properties_editor.cpp
    src/ui/panes/custom_properties_editor.h
    src/ui/panes/text_parser_editor.cpp
    src/ui/panes/text_parser_editor.h
    src/ui/panes/lip_sync_editor.cpp
    src/ui/panes/lip_sync_editor.h
    src/ui/panes/speech_pane.cpp
    src/ui/panes/speech_pane.h
    src/ui/panes/default_setup_pane.cpp
    src/ui/panes/default_setup_pane.h
    src/ui/panes/debug_log_pane.cpp
    src/ui/panes/debug_log_pane.h
    src/ui/panes/find_results_pane.cpp
    src/ui/panes/find_results_pane.h
)

# Shared sources from Tools/data (libtools)
set(TOOLS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/dialogscriptconv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/dialogscriptconv.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/script_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/script_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/game_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/scriptgen.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/scriptgen.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/tra_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/tra_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/mfl_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/mfl_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/room_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../data/room_utils.h
)

target_sources(editor PRIVATE
    ${IMGUI_SOURCES}
    ${IMGUI_FILE_DIALOG_SOURCES}
    ${IMGUI_COLOR_TEXT_EDIT_SOURCES}
    ${EDITOR_SOURCES}
    ${TOOLS_SOURCES}
)

target_include_directories(editor PRIVATE
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${IMGUI_FILE_DIALOG_DIR}
    ${IMGUI_COLOR_TEXT_EDIT_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Common
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Compiler
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libsrc/tinyxml2
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Define SOURCE_DIR so the editor can find its font/resource files at runtime
target_compile_definitions(editor PRIVATE
    SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Link against AGS Common and Compiler libraries
target_link_libraries(editor PRIVATE
    AGS::Common
    AGS::Compiler
    tinyxml2
    ${SDL2_LIBRARY}
    FreeType::FreeType
)

# Platform-specific settings
if(WIN32)
    target_link_libraries(editor PRIVATE shlwapi)
    # Use WinMain entry point on Windows for non-console app
    # set_target_properties(editor PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(LINUX)
    target_link_libraries(editor PRIVATE ${CMAKE_DL_LIBS})
elseif(MACOS)
    target_link_libraries(editor PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
    )
endif()

# Make sure SDL2 headers are available
target_include_directories(editor PRIVATE ${SDL2_INCLUDE_DIRS})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Source Files" FILES ${EDITOR_SOURCES})
source_group("Dear ImGui" FILES ${IMGUI_SOURCES})

if(AGS_DESKTOP)
    install(TARGETS editor RUNTIME DESTINATION bin)

    # Install desktop file (Linux)
    if(LINUX)
        install(FILES packaging/ags-editor.desktop
                DESTINATION share/applications)
    endif()

    # Install font files needed at runtime
    install(DIRECTORY imgui/misc/fonts/
            DESTINATION share/ags-editor/fonts
            FILES_MATCHING PATTERN "*.ttf")
endif()
