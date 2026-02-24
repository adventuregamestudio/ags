// AGS Editor ImGui - Game data structures
#pragma once

#include <string>
#include <vector>
#include <map>
#include <climits>

namespace AGSEditor
{

// Generic folder tree node — stores item IDs for any entity type.
// Used to preserve AGF folder hierarchy for round-trip.
struct FolderInfo {
    std::string name;
    std::vector<int> item_ids;            // entity IDs in this folder
    std::vector<FolderInfo> subfolders;
};

// Interaction event schema - defines the event slots for each entity type
struct InteractionEvent {
    const char* suffix;        // e.g. "Look", "Interact"
    const char* display_name;  // e.g. "Look at character"
    const char* params;        // e.g. "Character *theCharacter, CursorMode mode"
};

// Interaction data - stores which script functions are bound to events
struct Interactions {
    std::string script_module;  // which script file contains the handlers
    std::vector<std::string> handler_functions;  // one per event slot, empty = not assigned
};

// Interaction schemas for each entity type
namespace InteractionSchemas {

inline const std::vector<InteractionEvent>& Character() {
    static const std::vector<InteractionEvent> schema = {
        {"Look",     "Look at character",               "Character *theCharacter, CursorMode mode"},
        {"Interact", "Interact character",               "Character *theCharacter, CursorMode mode"},
        {"Talk",     "Talk to character",                "Character *theCharacter, CursorMode mode"},
        {"UseInv",   "Use inventory on character",       "Character *theCharacter, CursorMode mode"},
        {"AnyClick", "Any click on character",           "Character *theCharacter, CursorMode mode"},
        {"PickUp",   "Pick up character",                "Character *theCharacter, CursorMode mode"},
        {"Mode8",    "Mode 8 on character",              "Character *theCharacter, CursorMode mode"},
        {"Mode9",    "Mode 9 on character",              "Character *theCharacter, CursorMode mode"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& InventoryItem() {
    static const std::vector<InteractionEvent> schema = {
        {"Look",       "Look at inventory",              "InventoryItem *theItem, CursorMode mode"},
        {"Interact",   "Interact inventory",              "InventoryItem *theItem, CursorMode mode"},
        {"Talk",       "Talk to inventory",               "InventoryItem *theItem, CursorMode mode"},
        {"UseInv",     "Use inventory on inventory",      "InventoryItem *theItem, CursorMode mode"},
        {"OtherClick", "Other click on inventory",        "InventoryItem *theItem, CursorMode mode"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& RoomHotspot() {
    static const std::vector<InteractionEvent> schema = {
        {"WalkOn",    "Stands on hotspot",               ""},
        {"Look",      "Look at hotspot",                 "Hotspot *theHotspot, CursorMode mode"},
        {"Interact",  "Interact hotspot",                 "Hotspot *theHotspot, CursorMode mode"},
        {"UseInv",    "Use inventory on hotspot",         "Hotspot *theHotspot, CursorMode mode"},
        {"Talk",      "Talk to hotspot",                  "Hotspot *theHotspot, CursorMode mode"},
        {"AnyClick",  "Any click on hotspot",             "Hotspot *theHotspot, CursorMode mode"},
        {"MouseMove", "Mouse moves over hotspot",         ""},
        {"PickUp",    "Pick up hotspot",                  "Hotspot *theHotspot, CursorMode mode"},
        {"Mode8",     "Mode 8 on hotspot",                "Hotspot *theHotspot, CursorMode mode"},
        {"Mode9",     "Mode 9 on hotspot",                "Hotspot *theHotspot, CursorMode mode"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& RoomObject() {
    static const std::vector<InteractionEvent> schema = {
        {"Look",     "Look at object",                   "Object *theObject, CursorMode mode"},
        {"Interact", "Interact object",                   "Object *theObject, CursorMode mode"},
        {"Talk",     "Talk to object",                    "Object *theObject, CursorMode mode"},
        {"UseInv",   "Use inventory on object",           "Object *theObject, CursorMode mode"},
        {"AnyClick", "Any click on object",               "Object *theObject, CursorMode mode"},
        {"PickUp",   "Pick up object",                    "Object *theObject, CursorMode mode"},
        {"Mode8",    "Mode 8 on object",                  "Object *theObject, CursorMode mode"},
        {"Mode9",    "Mode 9 on object",                  "Object *theObject, CursorMode mode"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& Room() {
    static const std::vector<InteractionEvent> schema = {
        {"LeaveLeft",   "Walks off left edge",           ""},
        {"LeaveRight",  "Walks off right edge",          ""},
        {"LeaveBottom", "Walks off bottom edge",         ""},
        {"LeaveTop",    "Walks off top edge",            ""},
        {"FirstLoad",   "First time enters room",        ""},
        {"RoomLoad",    "Enters room (before fadein)",   ""},
        {"RepExec",     "Repeatedly execute",            ""},
        {"AfterFadeIn", "Enters room (after fadein)",    ""},
        {"Leave",       "Leaves room (before fadeout)",  ""},
        {"Unload",      "Leaves room (after fadeout)",   ""},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& RoomRegion() {
    static const std::vector<InteractionEvent> schema = {
        {"Standing", "While standing on region",         "Region *theRegion"},
        {"WalksOnto","Walks onto region",                "Region *theRegion"},
        {"WalksOff", "Walks off region",                 "Region *theRegion"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& NormalGUI() {
    static const std::vector<InteractionEvent> schema = {
        {"OnClick", "Click on GUI", "GUI *theGui, MouseButton button"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& GUIButton() {
    static const std::vector<InteractionEvent> schema = {
        {"OnClick", "Click on button", "GUIControl *control, MouseButton button"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& GUIListBox() {
    static const std::vector<InteractionEvent> schema = {
        {"OnSelectionChanged", "Selection changed", "GUIControl *control"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& GUISlider() {
    static const std::vector<InteractionEvent> schema = {
        {"OnChange", "Value changed", "GUIControl *control"},
    };
    return schema;
}

inline const std::vector<InteractionEvent>& GUITextBox() {
    static const std::vector<InteractionEvent> schema = {
        {"OnActivate", "Activate", "GUIControl *control"},
    };
    return schema;
}

} // namespace InteractionSchemas

// Simplified game data representation
// This mirrors the essential data from AGS::Common game structures
// and will be connected to the actual AGS Common library types
struct GameData
{
    std::string game_title = "My Adventure Game";
    int resolution_width = 320;
    int resolution_height = 200;
    int color_depth = 32;
    int target_fps = 40;
    bool debug_mode = true;

    // Game identity
    int unique_id = 0;                 // Unique game identifier (from AGF <UniqueID>)
    std::string guid_string;           // GUID string (from AGF <GUIDAsString>)
    int player_character_id = 0;       // Index of the player character

    // Additional settings matching C# editor GeneralSettings
    bool anti_alias_fonts = true;
    int starting_room = 1;
    std::string description;
    std::string developer_name;
    std::string developer_url;
    bool save_screenshots = false;
    int max_score = 0;

    // Visual settings
    bool pixel_perfect_speed = false;  // AntiGlideMode
    bool pixel_perfect = true;         // PixelPerfect click detection
    bool split_resources = false;
    int split_resource_threshold = 1024;  // KB
    int sprite_cache_size = 128;  // MB

    // Dialog settings
    int dialog_options_gui = -1;
    int dialog_options_gap = 0;
    int dialog_bullet_sprite = 0;
    int number_dialog_options = 0;     // DialogOptionNumbering enum: -1=none, 0=keys, 1=numbered
    bool dialog_options_backwards = false;
    bool run_game_loops_in_dialog = false;

    // Sound settings
    bool play_sound_on_score = true;
    int score_sound_clip = -1;
    int crossfade_music = 0;   // 0=no, 1=auto

    // Text & Speech
    int text_alignment = 0;   // 0=left, 1=center, 2=right
    bool use_speech = false;
    int speech_style = 0;     // 0=lucasarts, 1=sierra, 2=sierraBackground, 3=fullscreen
    int speech_portrait_side = 0; // 0=left, 1=right, 2=alternate, 3=position

    // Backwards compatibility
    bool enforce_object_scripting = false;
    bool enforce_new_strings = true;
    bool left_to_right_precedence = true;
    bool enforce_new_audio = true;
    int script_api_version = INT_MAX;  // kScriptAPI_* value: INT_MAX=Highest, 0=v321
    int script_compat_level = INT_MAX;  // kScriptAPI_* value: INT_MAX=Highest, 0=v321
    bool use_old_custom_dialog_api = false;
    bool use_old_keyboard_handling = false;

    // Project version info (from AGF root element)
    std::string saved_editor_version;  // e.g. "3.3.3", "3.6.1" — empty if not present

    // Text output
    int text_window_gui = -1;         // GUI to use as text window background
    int thought_gui = -1;             // GUI to use for character thinking
    bool always_display_text_as_speech = false;

    // Character behavior
    bool turn_before_walking = true;
    bool turn_before_facing = true;
    bool walk_in_look_mode = false;

    // Inventory
    bool inventory_cursors = true;     // use inventory item graphic as cursor
    bool handle_inv_clicks_in_script = true;
    bool display_multiple_inv = false; // show multiple icons for multiple items
    int inventory_hotspot_marker_style = 0; // 0=crosshair, 1=sprite
    int inventory_hotspot_marker_sprite = 0;
    int inventory_hotspot_dot_color = 0;
    int inventory_hotspot_crosshair_color = 0;

    // Saved games
    std::string save_game_extension;
    std::string save_game_folder;

    // Compiler extras
    std::string game_file_name = "Game";
    int sprite_file_compression = 0;   // 0=none, 1=RLE, 2=LZW, 3=Deflate

    // Room defaults
    int default_room_mask_resolution = 1;  // 1=1:1, 2=1:2, etc.

    // Visual extras
    int room_transition = 0;           // 0=cut, 1=fade, 2=dissolve, etc.
    int render_at_screen_resolution = 0; // RenderAtScreenRes enum: 0=user, 1=enabled, 2=disabled
    int when_interface_disabled = 0;   // 0=grey out controls, 1=hide GUIs, etc.
    bool letterbox_mode = false;
    int gui_alpha_style = 2;           // 0=Legacy, 1=AdditiveAlpha, 2=Proper
    int sprite_alpha_style = 1;        // 0=Legacy, 1=Proper
    bool mouse_wheel_enabled = true;
    bool auto_move_in_walk_mode = true;
    bool backwards_text = false;
    bool clip_gui_controls = true;
    int game_text_encoding = 0;        // Code page
    bool scale_char_sprite_offsets = false;
    bool use_old_voice_clip_naming = false;
    bool gui_handle_only_left_mouse = false;

    // Dialog extras
    std::string dialog_say_function;
    std::string dialog_narrate_function;
    int skip_speech_style = 0;        // 0=any key/mouse, 1=any key, 2=mouse only, 3=timer only
    bool use_global_speech_anim_delay = false;
    int global_speech_anim_delay = 5;

    // Global messages (legacy AGS 2.x, 500 entries, IDs 500-999)
    static constexpr int NUM_GLOBAL_MESSAGES = 500;
    static constexpr int GLOBAL_MESSAGE_ID_START = 500;
    std::string global_messages[500];

    // Placeholder lists (will be populated from actual game data)
    struct ScriptModule {
        std::string name;
        std::string header_file;
        std::string script_file;
    };
    std::vector<ScriptModule> script_modules;

    struct RoomInfo {
        int number;
        std::string description;
    };
    std::vector<RoomInfo> rooms;

    struct CharacterInfo {
        int id;
        std::string script_name;
        std::string real_name;
        int room;
        int x, y;
        int normal_view = -1;
        int speech_view = -1;
        int idle_view = -1;
        int thinking_view = -1;
        int blinking_view = -1;
        // Movement properties
        int movement_speed = 3;
        int movement_speed_x = 3;
        int movement_speed_y = 3;
        bool uniform_movement_speed = true;
        int animation_delay = 0;
        bool solid = true;
        int blocking_width = 0;
        int blocking_height = 0;
        bool turn_before_walking = true;
        bool turn_when_facing = true;
        bool diagonal_loops = true;
        bool adjust_speed_with_scaling = true;
        bool movement_linked_to_animation = true;
        // Appearance properties
        int baseline = 0;
        bool use_room_area_scaling = true;
        bool clickable = true;
        int transparency = 0;
        int speech_animation_delay = 0;
        int speech_color = 0;
        bool use_room_area_lighting = true;
        bool scale_volume = false;
        int idle_delay = 20;
        int idle_anim_speed = 5;
        bool is_player = false;
        // Event handlers
        Interactions interactions;
        // Custom properties (name -> value)
        std::map<std::string, std::string> custom_properties;
    };
    std::vector<CharacterInfo> characters;
    FolderInfo character_folders{"Main", {}, {}};

    // Sprite import transparency method (matches C# SpriteImportTransparency enum)
    enum class SpriteImportTransparency {
        PaletteIndex0 = 0,
        TopLeft = 1,
        BottomLeft = 2,
        TopRight = 3,
        BottomRight = 4,
        LeaveAsIs = 5,
        NoTransparency = 6,
        PaletteIndex = 7
    };

    // Sprite source / import metadata (round-tripped from AGF XML <Source> element)
    struct SpriteSourceInfo {
        std::string source_file;    // relative path to source image
        int frame = 0;              // frame index (for multi-frame sources like GIF)
        int offset_x = 0;          // crop offset X
        int offset_y = 0;          // crop offset Y
        int import_width = 0;      // crop width (0 = full)
        int import_height = 0;     // crop height (0 = full)
        bool import_as_tile = false;
        bool import_alpha_channel = false;
        SpriteImportTransparency transparency = SpriteImportTransparency::LeaveAsIs;
        int transparent_color_index = 0;
        bool remap_to_game_palette = false;
        bool remap_to_room_palette = false;

        bool HasSource() const { return !source_file.empty(); }
    };

    struct SpriteInfo {
        int id = 0;
        int width = 0, height = 0;
        int color_depth = 0;
        std::string resolution;     // "Real", "LowRes", "HighRes"
        bool alpha_channel = false;
        int colours_locked_to_room = -1;  // -1 = not locked (null in C#)
        SpriteSourceInfo source;
    };
    std::vector<SpriteInfo> sprites;

    // Sprite folder tree — preserves AGF folder hierarchy for round-trip
    // Uses SpriteFolderInfo alias for backward compatibility
    using SpriteFolderInfo = FolderInfo;
    SpriteFolderInfo root_sprite_folder{"Main", {}, {}};

    // Pending sprite metadata from AGF parsing, applied later when sprites
    // are populated from the binary sprite file in InitSubsystems.
    std::map<int, SpriteSourceInfo> pending_sprite_source_;
    std::map<int, std::string> pending_sprite_resolution_;
    std::map<int, bool> pending_sprite_alpha_;
    std::map<int, int> pending_sprite_colours_locked_;

    // Single animation frame
    struct FrameData {
        int sprite_id = 0;
        int x_offset = 0;
        int y_offset = 0;
        int delay = 0;      // additional frame delay
        bool flipped = false;
        int sound = -1;
    };

    // Animation loop
    struct LoopData {
        std::vector<FrameData> frames;
        bool run_next_loop = false;
    };

    struct ViewInfo {
        int id;
        std::string name;
        int loop_count;
        std::vector<LoopData> loops;
    };
    std::vector<ViewInfo> views;
    FolderInfo view_folders{"Main", {}, {}};

    // Look up a view by its ID (not vector index).
    // Returns nullptr if not found.
    ViewInfo* FindViewById(int view_id)
    {
        for (auto& v : views)
            if (v.id == view_id) return &v;
        return nullptr;
    }
    const ViewInfo* FindViewById(int view_id) const
    {
        for (auto& v : views)
            if (v.id == view_id) return &v;
        return nullptr;
    }

    struct DialogInfo {
        int id;
        std::string name;         // Display name (e.g. "Talk to Syd")
        std::string script_name;  // Script identifier (e.g. "dTalkSyd")
        std::string script; // Dialog script source (from AGF CDATA)
        int option_count;
        bool show_text_parser = false;
        struct DialogOption {
            std::string text;
            bool show = true;
            bool say = true;
        };
        std::vector<DialogOption> options;
    };
    std::vector<DialogInfo> dialogs;
    FolderInfo dialog_folders{"Main", {}, {}};

    struct GUIInfo {
        int id;
        std::string name;
        int x = 0, y = 0;
        int width = 100, height = 100;
        bool visible = true;
        bool clickable = true;
        int bg_color = 0;
        int border_color = 15;
        int bg_image = 0;       // background sprite ID (0 = none)
        int transparency = 0;   // 0-100 percentage
        int z_order = 0;
        int popup_style = 0;    // 0=normal, 1=mouse-y, 2=script-only/popup-modal, 3=persistent
        int popup_at_mouse_y = -1; // mouse Y position threshold for popup_style==1
        int padding = 3;        // text window padding
        std::string tag_name;   // "GUIMain" or "GUITextWindow"
        std::string on_click;   // GUI-level OnClick handler
        std::string script_module;  // script file for event handlers

        struct ControlInfo {
            int id = 0;
            std::string name;
            std::string type_tag;  // "GUIButton", "GUILabel", etc.
            int x = 0, y = 0;
            int width = 60, height = 20;
            std::string text;
            int image = -1;
            int font = 0;
            std::string event_handler;  // OnClick/OnSelectionChanged/OnChange/OnActivate
            // Per-control flags
            bool enabled = true;
            bool ctrl_visible = true;
            bool ctrl_clickable = true;
            bool translated = false;
            // Button-specific
            int mouseover_image = 0;
            int pushed_image = 0;
            int text_color = 0;
            int click_action = 0;    // 0=None, 1=SetMode, 2=RunScript
            int new_mode_number = 0;
            int text_alignment = 0;
            bool clip_image = false;
            bool wrap_text = false;
            // Label-specific
            // (text_color, text_alignment shared with button)
            // Slider-specific
            int min_value = 0;
            int max_value = 10;
            int slider_value = 0;
            int handle_image = 0;
            int handle_offset = 0;
            int bg_image = 0;
            // InvWindow-specific
            int inv_char_id = -1;
            int item_width = 40;
            int item_height = 22;
            // ListBox-specific
            int selected_text_color = 0;
            int selected_bg_color = 0;
            bool show_border = true;
            bool show_scroll_arrows = true;
            bool solid_background = false;
        };
        std::vector<ControlInfo> controls;
    };
    std::vector<GUIInfo> guis;
    FolderInfo gui_folders{"Main", {}, {}};

    struct FontInfo {
        int id;
        std::string name;
        int size;
        std::string source_file;
        int outline_type = 0;    // 0=none, 1=auto, 2=use font
        int outline_font = -1;
        int line_spacing = 0;
        int size_multiplier = 1;
    };
    std::vector<FontInfo> fonts;

    struct AudioClipInfo {
        int id;
        std::string name;
        std::string filename;
        std::string source_filename;  // original source path
        int type; // AudioClipType ID
        int default_volume = -1;  // -1 = inherit
        int default_priority = 0; // 0=inherit, 1=normal, 2=high, 3=low
        int default_repeat = 0;   // 0=inherit, 1=true, 2=false
        int bundling_type = 0;    // 0=InGameEXE, 1=InSeparateVOX
        int file_type = 0;        // AudioFileType enum
        std::string file_last_modified; // UTC timestamp string
    };
    std::vector<AudioClipInfo> audio_clips;
    FolderInfo audio_clip_folders{"Main", {}, {}};

    struct AudioClipTypeInfo {
        int id = 0;
        std::string name;
        int max_channels = 0;
        int volume_reduction_while_speech = 0;
        int crossfade_speed = 0; // 0=No, 1=Slow, 2=Fast
        bool backwards_compat_type = false;
    };
    std::vector<AudioClipTypeInfo> audio_clip_types;

    struct InventoryItemInfo {
        int id;
        std::string script_name;
        std::string description;
        int image;
        int cursor_image;
        int hotspot_x = 0;
        int hotspot_y = 0;
        bool start_with;
        // Event handlers
        Interactions interactions;
        // Custom properties (name -> value)
        std::map<std::string, std::string> custom_properties;
    };
    std::vector<InventoryItemInfo> inventory_items;
    FolderInfo inventory_folders{"Main", {}, {}};

    struct CursorInfo {
        int id;
        std::string name;
        int image;
        int hotspot_x, hotspot_y;
        bool animate;
        int view;
        bool process_click;
        bool animate_only_on_hotspot = false;
        bool animate_only_when_moving = false;
        int animation_delay = 5;
    };
    std::vector<CursorInfo> cursors;

    // 256-color palette
    struct PaletteEntry {
        int r, g, b;
        int colour_type;  // 0=Gamewide, 1=Locked, 2=Background
    };
    std::vector<PaletteEntry> palette;

    // Global Variables
    struct GlobalVariableInfo {
        std::string name;
        std::string type_name = "int"; // "int", "String", "float", "bool", or managed type name
        std::string default_value;
        int array_type = 0;     // 0=None, 1=Array (static), 2=DynamicArray
        int array_size = 0;     // for static arrays
    };
    std::vector<GlobalVariableInfo> global_variables;

    // Text Parser word groups
    struct TextParserWord {
        int word_group = 0;
        std::string word;
    };
    struct TextParserWordGroup {
        int id = 0;
        std::string name;
        std::vector<TextParserWord> words;
    };
    std::vector<TextParserWordGroup> text_parser_groups;

    // Lip Sync settings
    // Matches C# LipSync class: CharactersPerFrame is a fixed array of 20 strings,
    // each containing slash-separated characters mapped to that frame index.
    static const int kMaxLipSyncFrames = 20;
    int lip_sync_type = 0;            // 0=None, 1=Text, 2=PamelaVoiceFiles
    int default_lipsync_frame = 0;
    std::string lip_sync_chars_per_frame[kMaxLipSyncFrames];
    // Default values set in constructor/creation:
    // Frame 0: "A/I", Frame 1: "E", Frame 2: "O", Frame 3: "U",
    // Frame 4: "M/B/P", Frame 5: "C/D/G/K/N/R/S/Th/Y/Z",
    // Frame 6: "L", Frame 7: "F/V", Frame 8: "W/Q"

    // Custom property schemas
    struct CustomPropertySchemaInfo {
        std::string name;
        std::string description;
        int type = 0;                    // 0=bool, 1=int, 2=String
        std::string default_value;
        bool applies_to_characters = true;
        bool applies_to_hotspots = true;
        bool applies_to_objects = true;
        bool applies_to_rooms = true;
        bool applies_to_inv_items = true;
        bool translated = false;         // Whether text values are extracted for translation
    };
    std::vector<CustomPropertySchemaInfo> custom_property_schemas;

    // Runtime Setup (Default Setup pane) - winsetup equivalent
    struct RuntimeSetup {
        // Graphics
        int graphics_driver = 1; // 0=Software, 1=D3D9, 2=OpenGL
        bool windowed = false;
        bool fullscreen_desktop = true;
        int fullscreen_scaling = 1;  // 0=None, 1=ProportionalStretch, 2=StretchToFit, 3=Integer
        int windowed_scaling = 3;    // 0=None, 1=MaxRound, 2=StretchToFit, 3=MaxInteger
        int scaling_multiplier = 1;
        int graphics_filter = 0; // 0=Nearest, 1=Linear
        bool vsync = false;
        bool aa_scaled_sprites = false;
        bool render_at_screen_res = false;
        int rotation = 0; // 0=Unlocked, 1=Portrait, 2=Landscape

        // Audio
        int digital_sound = -1;  // -1=Default, 0=Disabled
        bool use_voice_pack = true;

        // Gameplay
        std::string translation;

        // Mouse
        bool auto_lock_mouse = false;
        float mouse_speed = 1.0f;

        // Touch
        int touch_emulation = 1;  // 0=Off, 1=OneFinger, 2=TwoFingers
        int touch_motion = 0;     // 0=Direct, 1=Relative

        // Misc
        bool show_fps = false;

        // Performance
        int sprite_cache_size = 128;   // MB
        int texture_cache_size = 128;  // MB
        int sound_cache_size = 32;     // MB
        bool compress_saves = true;

        // Environment
        bool use_custom_save_path = false;
        std::string custom_save_path;
        bool use_custom_appdata_path = false;
        std::string custom_appdata_path;

        // Setup appearance
        std::string title_text;
    };
    RuntimeSetup runtime_setup;
};

} // namespace AGSEditor
