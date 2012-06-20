
//=============================================================================
//
// Registering symbols for the script system
//
//=============================================================================

#include "script/symbol_registry.h"

void setup_script_exports() {

    register_audio_script_functions();
    register_button_script_functions();
    register_character_script_functions();
    register_datetime_script_functions();
    register_dialog_script_functions();
    register_drawingsurface_script_functions();
    register_dynamicsprite_script_functions();
    register_file_script_functions();
    register_game_script_functions();
    register_gui_script_functions();
    register_guicontrol_script_functions();
    register_hotspot_script_functions();
    register_inventoryitem_script_functions();
    register_invwindow_script_functions();
    register_label_script_functions();
    register_listbox_script_functions();
    register_math_script_functions();
    register_mouse_script_functions();
    register_object_script_functions();
    register_overlay_script_functions();
    register_parser_script_functions();
    register_region_script_functions();
    register_room_script_functions();
    register_slider_script_functions();
    register_string_script_functions();
    register_system_script_functions();
    register_textbox_script_functions();
    register_viewframe_script_functions();
    //-------------------------------------------------------------
    register_global_script_functions();
    register_builtin_plugins_script_functions();
}
