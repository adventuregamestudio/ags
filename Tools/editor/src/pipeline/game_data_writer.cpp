// AGS Editor ImGui - Game Data Writer (ac2game.dta)
// Writes game data in the exact binary format the AGS engine expects.
// C++ reimplementation of C# DataFileWriter.SaveThisGameToFile().
//
// Reference files:
//   Editor/AGS.Editor/DataFileWriter.cs (C# writer)
//   Common/game/main_game_file.cpp      (C++ reader)
//   Common/ac/gamesetupstructbase.cpp   (struct reader/writer)
//   Common/ac/gamestructdefines.h       (OPT_*, CHF_*, etc.)
//   Common/ac/characterinfo.h           (CHF_* flags, MAX_INV)
//   Common/ac/mousecursor.h             (MCF_* flags)
//   Common/gui/guidefines.h             (GUI constants)

#include "game_data_writer.h"
#include "project/game_data.h"
#include "pipeline/build_system.h"   // for BuildResult

#include "util/filestream.h"
#include "util/file.h"
#include "util/string_utils.h"
#include "script/cc_script.h"
#include "ac/gamestructdefines.h"
#include "ac/characterinfo.h"
#include "ac/mousecursor.h"
#include "ac/wordsdictionary.h"

#include <algorithm>
#include <cstring>
#include <memory>

using AGS::Common::Stream;

namespace AGSEditor {

// -------------------------------------------------------------------------
// Constants matching the AGS engine
// -------------------------------------------------------------------------
static const char GAME_FILE_SIG[] = "Adventure Creator Game File v2";
static const int32_t GAME_DATA_VERSION_CURRENT = 3060308; // kGameVersion_363_08
static const char ENGINE_VERSION[] = "3.6.3.8";

static const uint32_t GUIMAGIC = 0xCAFEBEEF;
static const int32_t GUI_VERSION = 3060308;

// MAXGLOBALMES (500) is defined in gamestructdefines.h
static const int MAXLIPSYNCFRAMES_VAL = 20;
static const int MAX_GUID_LENGTH_VAL = 40;
static const int MAX_SG_EXT_LENGTH_VAL = 20;
static const int LEGACY_MAX_SG_FOLDER_LEN_VAL = 50;
// LEGACY_MAX_SCRIPT_NAME_LEN (20) and LEGACY_MAX_CHAR_NAME_LEN (40)
// are already defined as macros in characterinfo.h
static const int LEGACY_CURSOR_NAME_LEN = 10;
static const int LEGACY_INV_NAME_LEN = 25;

// Font outline constants
static const int FONT_OUTLINE_NONE_VAL = -1;
static const int FONT_OUTLINE_AUTO_VAL = -10;

// GUI control type IDs
static const int kGUIButton = 1;
static const int kGUILabel = 2;
static const int kGUIInvWindow = 3;
static const int kGUISlider = 4;
static const int kGUITextBox = 5;
static const int kGUIListBox = 6;

// GUI flags
static const int kGUIMain_Clickable   = 0x0001;
static const int kGUIMain_TextWindow  = 0x0002;
static const int kGUIMain_Visible     = 0x0004;
static const int kGUICtrl_Enabled     = 0x0004;
static const int kGUICtrl_Visible     = 0x0010;
static const int kGUICtrl_Clip        = 0x0020;
static const int kGUICtrl_Clickable   = 0x0040;
static const int kGUICtrl_Translated  = 0x0080;
static const int kGUICtrl_WrapText    = 0x0100;
static const int kGUICtrl_ShowBorder  = 0x0200;
static const int kGUICtrl_SolidBack   = 0x0400;

// ListBox flags
static const int kListBox_ShowBorder  = 0x01;
static const int kListBox_ShowArrows  = 0x02;

// TextBox flags
static const int kTextBox_ShowBorder  = 0x0001;

// Transparency conversion: 0-100 percentage to legacy 0-255 range
// Matches Common/gfx/gfx_def.h Trans100ToLegacyTrans255()
static int Trans100ToLegacyTrans255(int transparency)
{
    if (transparency == 0) return 0;    // opaque
    if (transparency >= 100) return 255; // invisible
    // rest of range: alpha = (100 - trans) * 25 / 10
    return ((100 - transparency) * 25) / 10;
}

// Dialog flags
static const int DTFLG_SHOWPARSER = 0x0001;
static const int DFLG_ON = 0x0001;
static const int DFLG_NOREPEAT = 0x0004;

// Custom property version
static const int kPropertyVersion_Current = 2;

// -------------------------------------------------------------------------
// Helper: write a fixed-length string (padded with zeros)
// -------------------------------------------------------------------------
static void WriteFixedString(Stream* out, const std::string& s, int len)
{
    int write_len = std::min((int)s.size(), len);
    if (write_len > 0)
        out->Write(s.c_str(), write_len);
    // Pad remaining bytes with zeros
    for (int i = write_len; i < len; i++)
        out->WriteInt8(0);
}

// -------------------------------------------------------------------------
// Helper: write a length-prefixed string (FilePutString format)
// Format: int32 length, then length bytes (no NUL terminator)
// -------------------------------------------------------------------------
static void FilePutString(Stream* out, const std::string& s)
{
    out->WriteInt32((int32_t)s.size());
    if (!s.empty())
        out->Write(s.c_str(), s.size());
}

// -------------------------------------------------------------------------
// Helper: write a NUL-terminated string
// -------------------------------------------------------------------------
static void WriteNulTermString(Stream* out, const std::string& s)
{
    if (s.empty()) {
        out->WriteInt8(0);
    } else {
        out->Write(s.c_str(), s.size());
        out->WriteInt8(0);
    }
}

// -------------------------------------------------------------------------
// Helper: compute character flags from GameData::CharacterInfo
// -------------------------------------------------------------------------
static int32_t ComputeCharFlags(const GameData::CharacterInfo& ch)
{
    int32_t flags = 0;
    if (!ch.use_room_area_scaling) flags |= CHF_MANUALSCALING;
    if (!ch.clickable) flags |= CHF_NOINTERACT;
    if (!ch.diagonal_loops) flags |= CHF_NODIAGONAL;
    if (!ch.use_room_area_lighting) flags |= CHF_NOLIGHTING;
    if (!ch.turn_before_walking) flags |= CHF_NOTURNWHENWALK;
    if (!ch.solid) flags |= CHF_NOBLOCKING;
    if (ch.adjust_speed_with_scaling) flags |= CHF_SCALEMOVESPEED;
    if (ch.scale_volume) flags |= CHF_SCALEVOLUME;
    if (ch.movement_linked_to_animation) flags |= CHF_ANTIGLIDE;
    if (ch.turn_when_facing) flags |= CHF_TURNWHENFACE;
    return flags;
}

// -------------------------------------------------------------------------
// Helper: compute cursor flags from GameData::CursorInfo
// -------------------------------------------------------------------------
static int8_t ComputeCursorFlags(const GameData::CursorInfo& c)
{
    int8_t flags = 0;
    if (c.animate_only_when_moving) flags |= MCF_ANIMMOVE;
    if (c.process_click) flags |= MCF_STANDARD;
    if (c.animate_only_on_hotspot) flags |= MCF_HOTSPOT;
    return flags;
}

// -------------------------------------------------------------------------
// Resolve the actual ScriptAPIVersion enum value for options[] writing
// INT_MAX means "Highest" which resolves to kScriptAPI_Current
// -------------------------------------------------------------------------
static int ResolveScriptAPI(int api_val)
{
    if (api_val == INT_MAX)
        return kScriptAPI_Current;
    return api_val;
}

// -------------------------------------------------------------------------
// Section: GameSetupStructBase
// -------------------------------------------------------------------------
static void WriteGameSetupStructBase(Stream* out, const GameData& gd,
                                      soff_t& ext_off_pos)
{
    // gamename[50] — fixed 50 bytes
    WriteFixedString(out, gd.game_title, 50);
    // 2 bytes alignment padding
    out->WriteInt8(0);
    out->WriteInt8(0);

    // options[100] — 100 int32 values
    int32_t options[100] = {};
    options[OPT_DEBUGMODE] = gd.debug_mode ? 1 : 0;
    // OPT_SCORESOUND (1) - left as 0
    options[OPT_WALKONLOOK] = gd.walk_in_look_mode ? 1 : 0;
    options[OPT_DIALOGIFACE] = gd.dialog_options_gui;
    options[OPT_ANTIGLIDE] = gd.pixel_perfect_speed ? 1 : 0;
    options[OPT_TWCUSTOM] = gd.text_window_gui;
    options[OPT_DIALOGGAP] = gd.dialog_options_gap;
    options[OPT_NOSKIPTEXT] = gd.skip_speech_style;
    options[OPT_DISABLEOFF] = gd.when_interface_disabled;
    options[OPT_ALWAYSSPCH] = gd.always_display_text_as_speech ? 1 : 0;
    options[OPT_SPEECHTYPE] = gd.speech_style;
    options[OPT_PIXPERFECT] = gd.pixel_perfect ? 1 : 0;
    options[OPT_NOWALKMODE] = gd.auto_move_in_walk_mode ? 0 : 1; // inverted
    options[OPT_LETTERBOX] = gd.letterbox_mode ? 1 : 0;
    options[OPT_FIXEDINVCURSOR] = gd.inventory_cursors ? 0 : 1; // inverted
    options[OPT_HIRES_FONTS] = 0; // always 0
    options[OPT_SPLITRESOURCES] = gd.split_resources ? gd.split_resource_threshold : 0;
    options[OPT_CHARTURNWHENWALK] = gd.turn_before_walking ? 1 : 0;
    options[OPT_FADETYPE] = gd.room_transition;
    options[OPT_HANDLEINVCLICKS] = gd.handle_inv_clicks_in_script ? 1 : 0;
    options[OPT_MOUSEWHEEL] = gd.mouse_wheel_enabled ? 1 : 0;
    options[OPT_DIALOGNUMBERED] = gd.number_dialog_options;
    options[OPT_DIALOGUPWARDS] = gd.dialog_options_backwards ? 1 : 0;
    // OPT_CROSSFADEMUSIC (24) - left as 0
    options[OPT_ANTIALIASFONTS] = gd.anti_alias_fonts ? 1 : 0;
    options[OPT_THOUGHTGUI] = gd.thought_gui;
    options[OPT_CHARTURNWHENFACE] = gd.turn_before_facing ? 1 : 0;
    options[OPT_RIGHTLEFTWRITE] = gd.backwards_text ? 1 : 0;
    options[OPT_DUPLICATEINV] = gd.display_multiple_inv ? 1 : 0;
    options[OPT_SAVESCREENSHOT] = gd.save_screenshots ? 1 : 0;
    options[OPT_PORTRAITSIDE] = gd.speech_portrait_side;
    options[OPT_STRICTSCRIPTING] = gd.enforce_object_scripting ? 1 : 0;
    options[OPT_LEFTTORIGHTEVAL] = gd.left_to_right_precedence ? 1 : 0;
    options[OPT_COMPRESSSPRITES] = gd.sprite_file_compression;
    options[OPT_STRICTSTRINGS] = gd.enforce_new_strings ? 1 : 0;
    options[OPT_NEWGUIALPHA] = gd.gui_alpha_style;
    options[OPT_RUNGAMEDLGOPTS] = gd.run_game_loops_in_dialog ? 1 : 0;
    options[OPT_NATIVECOORDINATES] = 1; // always use native coordinates
    // OPT_GLOBALTALKANIMSPD (39) - speech animation delay encoding
    if (gd.use_global_speech_anim_delay)
        options[OPT_GLOBALTALKANIMSPD] = gd.global_speech_anim_delay;
    else
        options[OPT_GLOBALTALKANIMSPD] = -(gd.global_speech_anim_delay) - 1;
    options[OPT_SPRITEALPHA] = gd.sprite_alpha_style;
    options[OPT_SAFEFILEPATHS] = 1; // always 1
    options[OPT_DIALOGOPTIONSAPI] = gd.use_old_custom_dialog_api ? -1 : 1;
    options[OPT_BASESCRIPTAPI] = ResolveScriptAPI(gd.script_api_version);
    options[OPT_SCRIPTCOMPATLEV] = ResolveScriptAPI(gd.script_compat_level);
    options[OPT_RENDERATSCREENRES] = gd.render_at_screen_resolution;
    options[OPT_RELATIVEASSETRES] = 0; // default
    options[OPT_WALKSPEEDABSOLUTE] = 1; // don't scale with mask
    options[OPT_CLIPGUICONTROLS] = gd.clip_gui_controls ? 1 : 0;
    options[OPT_GAMETEXTENCODING] = gd.game_text_encoding;
    options[OPT_KEYHANDLEAPI] = gd.use_old_keyboard_handling ? 0 : 1; // inverted
    // OPT_CUSTOMENGINETAG (51) - left as 0
    options[OPT_SCALECHAROFFSETS] = gd.scale_char_sprite_offsets ? 1 : 0;
    options[53] = -1; // OPT_SAVESCREENSHOTLAYER = all layers
    options[54] = gd.use_old_voice_clip_naming ? 0 : 1; // inverted
    // OPT_SAVECOMPONENTSIGNORE (55) - left as 0
    options[OPT_GAMEFPS] = gd.target_fps;
    options[57] = gd.gui_handle_only_left_mouse ? 1 : 0; // OPT_GUICONTROLMOUSEBUT
    options[OPT_LIPSYNCTEXT] = (gd.lip_sync_type == 1) ? 1 : 0;

    for (int i = 0; i < 100; i++)
        out->WriteInt32(options[i]);

    // paluses[256] — palette usage flags
    for (int i = 0; i < 256; i++) {
        if (i < (int)gd.palette.size())
            out->WriteInt8((uint8_t)gd.palette[i].colour_type);
        else
            out->WriteInt8(0); // PAL_GAMEWIDE
    }

    // defpal[256] — default palette (R/4, G/4, B/4, 0xFF)
    for (int i = 0; i < 256; i++) {
        if (i < (int)gd.palette.size()) {
            out->WriteInt8((uint8_t)(gd.palette[i].r / 4));
            out->WriteInt8((uint8_t)(gd.palette[i].g / 4));
            out->WriteInt8((uint8_t)(gd.palette[i].b / 4));
            out->WriteInt8((uint8_t)0xFF);
        } else {
            out->WriteInt8(0);
            out->WriteInt8(0);
            out->WriteInt8(0);
            out->WriteInt8((uint8_t)0xFF);
        }
    }

    // numviews
    out->WriteInt32((int32_t)gd.views.size());
    // numcharacters
    out->WriteInt32((int32_t)gd.characters.size());
    // playercharacter
    out->WriteInt32(gd.player_character_id);
    // totalscore
    out->WriteInt32(gd.max_score);
    // numinvitems (short) — includes dummy slot 0
    out->WriteInt16((int16_t)(gd.inventory_items.size() + 1));
    // 2 bytes padding
    out->WriteInt16(0);
    // numdialog — written as 0 (actual count in extension v363_dialogsnew)
    out->WriteInt32(0);
    // numdlgmessage — always 0
    out->WriteInt32(0);
    // numfonts
    out->WriteInt32((int32_t)gd.fonts.size());
    // color_depth — engine expects bytes per pixel (1, 2, 4)
    out->WriteInt32(gd.color_depth / 8);
    // target_win — always 0
    out->WriteInt32(0);
    // dialog_bullet
    out->WriteInt32(gd.dialog_bullet_sprite);
    // hotdot (short)
    out->WriteInt16((int16_t)gd.inventory_hotspot_dot_color);
    // hotdotouter (short)
    out->WriteInt16((int16_t)gd.inventory_hotspot_crosshair_color);
    // uniqueid
    out->WriteInt32(gd.unique_id);
    // numgui
    out->WriteInt32((int32_t)gd.guis.size());
    // numcursors
    out->WriteInt32((int32_t)gd.cursors.size());

    // default_resolution — always custom (8) + width + height
    out->WriteInt32(kGameResolution_Custom);
    out->WriteInt32(gd.resolution_width);
    out->WriteInt32(gd.resolution_height);

    // default_lipsync_frame
    out->WriteInt32(gd.default_lipsync_frame);
    // invhotdotsprite
    out->WriteInt32(gd.inventory_hotspot_marker_sprite);

    // reserved[16]
    for (int i = 0; i < 16; i++)
        out->WriteInt32(0);

    // ext_off placeholder — will be backpatched later
    ext_off_pos = out->GetPosition();
    out->WriteInt32(0); // placeholder

    // HasMessages[500]
    for (int i = 0; i < MAXGLOBALMES; i++)
        out->WriteInt32(gd.global_messages[i].empty() ? 0 : 1);

    // dict_flag = 1 (dictionary present)
    out->WriteInt32(1);
    // globalscript_flag = 0
    out->WriteInt32(0);
    // chars_flag = 0
    out->WriteInt32(0);
    // compiled_script_flag = 1
    out->WriteInt32(1);
}

// -------------------------------------------------------------------------
// Section: Save Game Info
// -------------------------------------------------------------------------
static void WriteSaveGameInfo(Stream* out, const GameData& gd)
{
    // GUID string (40 bytes fixed)
    WriteFixedString(out, gd.guid_string, MAX_GUID_LENGTH_VAL);
    // Save game extension (20 bytes fixed)
    WriteFixedString(out, gd.save_game_extension, MAX_SG_EXT_LENGTH_VAL);
    // Save game folder name (50 bytes fixed)
    WriteFixedString(out, gd.save_game_folder, LEGACY_MAX_SG_FOLDER_LEN_VAL);
}

// -------------------------------------------------------------------------
// Section: Font Info
// -------------------------------------------------------------------------
static void WriteFontInfos(Stream* out, const GameData& gd)
{
    for (const auto& f : gd.fonts) {
        // flags — match C# editor behavior:
        //   WFN (PointSize==0): FFLG_SIZEMULTIPLIER | FFLG_LOGICALNOMINALHEIGHT
        //   TTF (PointSize>0):  FFLG_LOGICALNOMINALHEIGHT | FFLG_ASCENDERFIXUP
        int32_t flags = 0;
        if (f.size == 0)
            flags |= FFLG_SIZEMULTIPLIER;
        flags |= FFLG_LOGICALNOMINALHEIGHT;
        if (f.size > 0)
            flags |= FFLG_ASCENDERFIXUP;
        if (f.line_spacing == 0) flags |= FFLG_DEFLINESPACING;
        out->WriteInt32(flags);

        // size field:
        //   If FFLG_SIZEMULTIPLIER: write SizeMultiplier (engine interprets as multiplier)
        //   Otherwise: write PointSize * SizeMultiplier (engine interprets as pixel size)
        if (flags & FFLG_SIZEMULTIPLIER)
            out->WriteInt32(std::max(f.size_multiplier, 1));
        else
            out->WriteInt32(f.size * std::max(f.size_multiplier, 1));

        // outline
        int outline = FONT_OUTLINE_NONE_VAL;
        if (f.outline_type == 1) outline = FONT_OUTLINE_AUTO_VAL;
        else if (f.outline_type == 2 && f.outline_font >= 0) outline = f.outline_font;
        out->WriteInt32(outline);

        // yOffset
        out->WriteInt32(0);

        // lineSpacing
        out->WriteInt32(f.line_spacing);
    }
}

// -------------------------------------------------------------------------
// Section: Sprite Flags
// -------------------------------------------------------------------------
static void WriteSpriteFlags(Stream* out, const GameData& gd)
{
    if (gd.sprites.empty()) {
        out->WriteInt32(0);
        return;
    }
    // Find topmost sprite number
    int topmost = 0;
    for (const auto& s : gd.sprites)
        if (s.id > topmost) topmost = s.id;

    int count = topmost + 1;
    out->WriteInt32(count);

    // Build flag array
    std::vector<uint8_t> flags(count, 0);
    for (const auto& s : gd.sprites) {
        if (s.id >= 0 && s.id < count) {
            uint8_t f = 0;
            if (s.color_depth > 16) f |= SPF_TRUECOLOR;
            else if (s.color_depth > 8) f |= SPF_HICOLOR;
            if (s.color_depth >= 32) f |= SPF_ALPHACHANNEL;
            flags[s.id] = f;
        }
    }
    out->Write(flags.data(), count);
}

// -------------------------------------------------------------------------
// Section: Inventory Items (legacy fixed struct)
// -------------------------------------------------------------------------
static void WriteInventoryItems(Stream* out, const GameData& gd)
{
    // Slot 0: dummy (68 bytes of zeros)
    for (int i = 0; i < 68; i++)
        out->WriteInt8(0);

    // Real inventory items
    for (const auto& inv : gd.inventory_items) {
        // name[25] — 24 chars + NUL (WriteString(24) + 1 NUL byte)
        std::string name = inv.description;
        if (name.size() > 24) name.resize(24);
        WriteFixedString(out, name, 24);
        out->WriteInt8(0); // NUL terminator

        // 3 bytes padding
        out->WriteInt8(0);
        out->WriteInt8(0);
        out->WriteInt8(0);

        // pic (int32)
        out->WriteInt32(inv.image);
        // cursorPic (int32)
        out->WriteInt32(inv.cursor_image);
        // hotx (int32)
        out->WriteInt32(inv.hotspot_x);
        // hoty (int32)
        out->WriteInt32(inv.hotspot_y);
        // reserved[5] (5 × int32)
        for (int r = 0; r < 5; r++)
            out->WriteInt32(0);
        // flags (int8) — IFLG_STARTWITH = 1
        out->WriteInt8(inv.start_with ? 1 : 0);
        // 3 bytes padding
        out->WriteInt8(0);
        out->WriteInt8(0);
        out->WriteInt8(0);
    }
}

// -------------------------------------------------------------------------
// Section: Mouse Cursors (legacy fixed struct, 24 bytes each)
// -------------------------------------------------------------------------
static void WriteMouseCursors(Stream* out, const GameData& gd)
{
    for (const auto& c : gd.cursors) {
        // pic (int32)
        out->WriteInt32(c.image);
        // hotx (int16)
        out->WriteInt16((int16_t)c.hotspot_x);
        // hoty (int16)
        out->WriteInt16((int16_t)c.hotspot_y);
        // view (int16) — 0-based, -1 if not animated
        out->WriteInt16((int16_t)(c.animate ? (c.view - 1) : -1));
        // name[10] — legacy script name
        std::string cname = c.name;
        if (cname.size() > 9) cname.resize(9);
        WriteFixedString(out, cname, LEGACY_CURSOR_NAME_LEN);
        // flags (int8)
        out->WriteInt8(ComputeCursorFlags(c));
        // 3 bytes padding
        out->WriteInt8(0);
        out->WriteInt8(0);
        out->WriteInt8(0);
    }
}

// -------------------------------------------------------------------------
// Section: Interaction Scripts (legacy dummy)
// -------------------------------------------------------------------------
static void WriteInteractionScripts(Stream* out, const GameData& gd)
{
    // One zero int per character (zero func count)
    for (size_t i = 0; i < gd.characters.size(); i++)
        out->WriteInt32(0);
    // One zero int per inventory item (excluding slot 0)
    for (size_t i = 0; i < gd.inventory_items.size(); i++)
        out->WriteInt32(0);
}

// -------------------------------------------------------------------------
// Section: Words Dictionary
// -------------------------------------------------------------------------
static void WriteWordsDictionary(Stream* out, const GameData& gd)
{
    // Collect all words from all groups
    struct WordEntry {
        std::string word;
        int group;
    };
    std::vector<WordEntry> all_words;
    for (const auto& grp : gd.text_parser_groups) {
        for (const auto& w : grp.words) {
            all_words.push_back({w.word, w.word_group});
        }
    }

    out->WriteInt32((int32_t)all_words.size());
    for (const auto& w : all_words) {
        write_string_encrypt(out, w.word.c_str());
        out->WriteInt16((int16_t)w.group);
    }
}

// Helper: read a compiled script from a .o file and write it to the output
static bool WriteScriptFromFile(Stream* out, const std::string& obj_file,
                                 const std::string& label, BuildResult& result)
{
    std::unique_ptr<Stream> in(AGS::Common::File::OpenFileRead(obj_file.c_str()));
    if (!in) {
        result.AddError("", 0, "Cannot open compiled script: " + obj_file);
        return false;
    }
    ccScript* script = ccScript::CreateFromStream(in.get());
    if (!script) {
        result.AddError("", 0, "Cannot read compiled " + label + ": " + obj_file);
        return false;
    }
    script->Write(out);
    delete script;
    return true;
}

// -------------------------------------------------------------------------
// Section: Compiled Scripts
// -------------------------------------------------------------------------
static bool WriteCompiledScripts(Stream* out, const GameData& gd,
                                  const std::vector<CompiledScriptRef>& scripts,
                                  BuildResult& result)
{
    // Order: global script (0), dialog script (1), then modules (2+)
    if (scripts.size() < 2) {
        result.AddError("", 0, "Not enough compiled scripts (need global + dialog)");
        return false;
    }

    // Write global script
    if (!WriteScriptFromFile(out, scripts[0].obj_file, "global script", result))
        return false;

    // Write dialog script
    if (!WriteScriptFromFile(out, scripts[1].obj_file, "dialog script", result))
        return false;

    // Script modules count
    int module_count = (int)scripts.size() - 2;
    out->WriteInt32(module_count);

    // Write each script module
    for (int i = 2; i < (int)scripts.size(); i++) {
        if (!WriteScriptFromFile(out, scripts[i].obj_file, "script module", result))
            return false;
    }

    return true;
}

// -------------------------------------------------------------------------
// Section: Views
// -------------------------------------------------------------------------
static void WriteViews(Stream* out, const GameData& gd)
{
    for (const auto& v : gd.views) {
        // numLoops (uint16)
        out->WriteInt16((uint16_t)v.loops.size());

        for (const auto& loop : v.loops) {
            // numFrames (uint16)
            out->WriteInt16((uint16_t)loop.frames.size());
            // loop flags (int32)
            out->WriteInt32(loop.run_next_loop ? 1 : 0); // LOOPFLAG_RUNNEXTLOOP

            for (const auto& frame : loop.frames) {
                // pic (int32)
                out->WriteInt32(frame.sprite_id);
                // xoffs (int16)
                out->WriteInt16((int16_t)frame.x_offset);
                // yoffs (int16)
                out->WriteInt16((int16_t)frame.y_offset);
                // speed (int16)
                out->WriteInt16((int16_t)frame.delay);
                // padding (int16)
                out->WriteInt16(0);
                // flags (int32) — VFLG_FLIPSPRITE = 1
                out->WriteInt32(frame.flipped ? 1 : 0);
                // sound (int32)
                out->WriteInt32(frame.sound);
                // reserved[0] (int32)
                out->WriteInt32(0);
                // reserved[1] (int32)
                out->WriteInt32(0);
            }
        }
    }
}

// -------------------------------------------------------------------------
// Section: Characters (756 bytes each)
// -------------------------------------------------------------------------
static void WriteCharacters(Stream* out, const GameData& gd)
{
    for (const auto& ch : gd.characters) {
        int32_t flags = ComputeCharFlags(ch);

        // Views are stored 0-based in the binary, -1 means none.
        // AGF stores 1-based (first view = 1, none = 0), so convert: value - 1.
        int defview = (ch.normal_view > 0) ? (ch.normal_view - 1) : -1;
        int talkview = (ch.speech_view > 0) ? (ch.speech_view - 1) : -1;
        int idleview = (ch.idle_view > 0) ? (ch.idle_view - 1) : -1;
        int thinkview = (ch.thinking_view > 0) ? (ch.thinking_view - 1) : -1;
        int blinkview = (ch.blinking_view > 0) ? (ch.blinking_view - 1) : -1;

        // defview (int32) — default/normal view
        out->WriteInt32(defview);
        // talkview (int32) — speech view
        out->WriteInt32(talkview);
        // view (int32) — current view (= defview at design time)
        out->WriteInt32(defview);
        // room (int32)
        out->WriteInt32(ch.room);
        // prevroom (int32) — 0 at design time
        out->WriteInt32(0);
        // x (int32)
        out->WriteInt32(ch.x);
        // y (int32)
        out->WriteInt32(ch.y);
        // wait (int32) — 0 at design time
        out->WriteInt32(0);
        // flags (int32)
        out->WriteInt32(flags);
        // following (int16) — 0
        out->WriteInt16(0);
        // followinfo (int16) — 0
        out->WriteInt16(0);
        // idleview (int32)
        out->WriteInt32(idleview);
        // idletime (int16)
        out->WriteInt16((int16_t)ch.idle_delay);
        // idleleft (int16) — 0
        out->WriteInt16(0);
        // transparency (int16)
        out->WriteInt16((int16_t)ch.transparency);
        // baseline (int16)
        out->WriteInt16((int16_t)ch.baseline);
        // activeinv (int32) — 0
        out->WriteInt32(0);
        // talkcolor (int32)
        out->WriteInt32(ch.speech_color);
        // thinkview (int32)
        out->WriteInt32(thinkview);
        // blinkview (int16)
        out->WriteInt16((int16_t)blinkview);
        // blinkinterval (int16) — 0
        out->WriteInt16(0);
        // blinktimer (int16) — 0
        out->WriteInt16(0);
        // blinkframe (int16) — 0
        out->WriteInt16(0);
        // walkspeed_y (int16) — UNIFORM_WALK_SPEED=0 if uniform
        out->WriteInt16(ch.uniform_movement_speed ?
            (int16_t)UNIFORM_WALK_SPEED : (int16_t)ch.movement_speed_y);
        // pic_yoffs (int16) — 0
        out->WriteInt16(0);
        // z (int32) — 0
        out->WriteInt32(0);
        // walkwait (int32) — 0
        out->WriteInt32(0);
        // speech_anim_speed (int16)
        out->WriteInt16((int16_t)ch.speech_animation_delay);
        // idle_anim_speed (int16)
        out->WriteInt16((int16_t)ch.idle_anim_speed);
        // blocking_width (int16)
        out->WriteInt16((int16_t)ch.blocking_width);
        // blocking_height (int16)
        out->WriteInt16((int16_t)ch.blocking_height);
        // index_id (int32)
        out->WriteInt32(ch.id);
        // blocking_x (int16) — 0
        out->WriteInt16(0);
        // blocking_y (int16) — 0
        out->WriteInt16(0);
        // loop (int16) — 0
        out->WriteInt16(0);
        // frame (int16) — 0
        out->WriteInt16(0);
        // walking (int16) — 0
        out->WriteInt16(0);
        // animating (int16) — 0
        out->WriteInt16(0);
        // walkspeed (int16)
        out->WriteInt16((int16_t)ch.movement_speed);
        // animspeed (int16)
        out->WriteInt16((int16_t)ch.animation_delay);

        // inv[MAX_INV=301] — inventory array (int16 each)
        // Set starting inventory items on the player character
        int16_t inv_array[MAX_INV] = {};
        if (ch.is_player) {
            for (const auto& item : gd.inventory_items) {
                if (item.start_with && item.id >= 1 && item.id < MAX_INV)
                    inv_array[item.id] = 1;
            }
        }
        for (int i = 0; i < MAX_INV; i++)
            out->WriteInt16(inv_array[i]);

        // actx (int16) — 0
        out->WriteInt16(0);
        // acty (int16) — 0
        out->WriteInt16(0);

        // name[40] — real name (fixed length)
        WriteFixedString(out, ch.real_name, LEGACY_MAX_CHAR_NAME_LEN);
        // scrname[20] — script name (fixed length)
        WriteFixedString(out, ch.script_name, LEGACY_MAX_SCRIPT_NAME_LEN);

        // on (int8) — character enabled (1)
        out->WriteInt8(1);
        // padding (1 byte)
        out->WriteInt8(0);
    }
}

// -------------------------------------------------------------------------
// Section: Lip-Sync Data (20 × 50-char strings)
// -------------------------------------------------------------------------
static void WriteLipSyncData(Stream* out, const GameData& gd)
{
    // Write CharactersPerFrame array (20 x 50-char strings)
    // This matches the C# editor format directly
    for (int f = 0; f < MAXLIPSYNCFRAMES_VAL; f++) {
        WriteFixedString(out, gd.lip_sync_chars_per_frame[f], 50);
    }
}

// -------------------------------------------------------------------------
// Section: Global Messages (encrypted, sparse)
// -------------------------------------------------------------------------
static void WriteGlobalMessages(Stream* out, const GameData& gd)
{
    // Write encrypted strings for each message where HasMessages[i] == 1
    for (int i = 0; i < MAXGLOBALMES; i++)
    {
        if (!gd.global_messages[i].empty())
            write_string_encrypt(out, gd.global_messages[i].c_str());
    }
}

// -------------------------------------------------------------------------
// Section: GUIs
// -------------------------------------------------------------------------

// Map control type tag to type ID
static int ControlTypeFromTag(const std::string& tag)
{
    if (tag == "GUIButton" || tag == "GUITextWindowEdge") return kGUIButton;
    if (tag == "GUILabel") return kGUILabel;
    if (tag == "GUIInventoryWindow" || tag == "GUIInventory") return kGUIInvWindow;
    if (tag == "GUISlider") return kGUISlider;
    if (tag == "GUITextBox") return kGUITextBox;
    if (tag == "GUIListBox") return kGUIListBox;
    return kGUIButton; // fallback
}

// Write GUIObject base for a control
static void WriteGUIObjectBase(Stream* out, const GameData::GUIInfo::ControlInfo& ctrl,
                                int type_id, bool has_event, int extra_flags = 0)
{
    // Build control flags from per-control properties
    int32_t flags = extra_flags;
    if (ctrl.enabled) flags |= kGUICtrl_Enabled;
    if (ctrl.ctrl_visible) flags |= kGUICtrl_Visible;
    if (ctrl.ctrl_clickable) flags |= kGUICtrl_Clickable;
    if (ctrl.translated) flags |= kGUICtrl_Translated;
    if (ctrl.show_border) flags |= kGUICtrl_ShowBorder;
    if (ctrl.solid_background) flags |= kGUICtrl_SolidBack;
    out->WriteInt32(flags);
    // x, y, width, height (int32 each)
    out->WriteInt32(ctrl.x);
    out->WriteInt32(ctrl.y);
    out->WriteInt32(ctrl.width);
    out->WriteInt32(ctrl.height);
    // zOrder (int32) — use control id as z-order
    out->WriteInt32(ctrl.id);
    // name (NUL-terminated string)
    WriteNulTermString(out, ctrl.name);
    // numEvents (int32)
    out->WriteInt32(has_event ? 1 : 0);
    // event handlers
    if (has_event)
        WriteNulTermString(out, ctrl.event_handler);
}

static void WriteGUIs(Stream* out, const GameData& gd)
{
    // GUI Magic
    out->WriteInt32((int32_t)GUIMAGIC);
    // GUI format version
    out->WriteInt32(GUI_VERSION);
    // GUI count
    out->WriteInt32((int32_t)gd.guis.size());

    // Collect controls by type into flat arrays
    struct ButtonInfo { const GameData::GUIInfo::ControlInfo* ctrl; };
    struct LabelInfo { const GameData::GUIInfo::ControlInfo* ctrl; };
    struct InvWindowInfo { const GameData::GUIInfo::ControlInfo* ctrl; };
    struct SliderInfo { const GameData::GUIInfo::ControlInfo* ctrl; };
    struct TextBoxInfo { const GameData::GUIInfo::ControlInfo* ctrl; };
    struct ListBoxInfo { const GameData::GUIInfo::ControlInfo* ctrl; };

    std::vector<const GameData::GUIInfo::ControlInfo*> buttons;
    std::vector<const GameData::GUIInfo::ControlInfo*> labels;
    std::vector<const GameData::GUIInfo::ControlInfo*> inv_windows;
    std::vector<const GameData::GUIInfo::ControlInfo*> sliders;
    std::vector<const GameData::GUIInfo::ControlInfo*> textboxes;
    std::vector<const GameData::GUIInfo::ControlInfo*> listboxes;

    // Per-GUI data
    for (const auto& gui : gd.guis) {
        bool is_text_window = (gui.tag_name == "GUITextWindow");

        // name (FilePutString)
        FilePutString(out, gui.name);
        // clickEventHandler (FilePutString) — NULL for TextWindowGUI
        if (is_text_window)
            FilePutString(out, "");
        else
            FilePutString(out, gui.on_click);

        // x, y, width, height (int32 each)
        out->WriteInt32(gui.x);
        out->WriteInt32(gui.y);
        out->WriteInt32(gui.width);
        out->WriteInt32(gui.height);
        // numobjs (int32)
        out->WriteInt32((int32_t)gui.controls.size());

        // popupStyle (int32)
        out->WriteInt32(gui.popup_style);

        // popupYPos (int32)
        out->WriteInt32(gui.popup_at_mouse_y);
        // bgcol (int32)
        out->WriteInt32(gui.bg_color);
        // bgpic (int32) — background image sprite
        out->WriteInt32(gui.bg_image);
        // fgcol (int32) — foreground/border/text color
        out->WriteInt32(gui.border_color);

        // flags (int32)
        int32_t gui_flags = 0;
        if (is_text_window) {
            gui_flags = kGUIMain_TextWindow;
        } else {
            if (gui.clickable) gui_flags |= kGUIMain_Clickable;
            if (gui.visible) gui_flags |= kGUIMain_Visible;
        }
        out->WriteInt32(gui_flags);

        // transparency (int32) — convert 0-100% to legacy 0-255 range
        out->WriteInt32(Trans100ToLegacyTrans255(gui.transparency));
        // zorder (int32)
        out->WriteInt32(gui.z_order);
        // guiId (int32) — written as 0
        out->WriteInt32(0);
        // padding (int32)
        out->WriteInt32(gui.padding);

        // Control references: objrefptr = (type << 16) | index
        for (const auto& ctrl : gui.controls) {
            int type_id = ControlTypeFromTag(ctrl.type_tag);
            int index = 0;
            switch (type_id) {
                case kGUIButton: index = (int)buttons.size(); buttons.push_back(&ctrl); break;
                case kGUILabel: index = (int)labels.size(); labels.push_back(&ctrl); break;
                case kGUIInvWindow: index = (int)inv_windows.size(); inv_windows.push_back(&ctrl); break;
                case kGUISlider: index = (int)sliders.size(); sliders.push_back(&ctrl); break;
                case kGUITextBox: index = (int)textboxes.size(); textboxes.push_back(&ctrl); break;
                case kGUIListBox: index = (int)listboxes.size(); listboxes.push_back(&ctrl); break;
            }
            out->WriteInt32((type_id << 16) | index);
        }
    }

    // --- Flat control arrays ---

    // Buttons (includes GUITextWindowEdge controls)
    out->WriteInt32((int32_t)buttons.size());
    for (const auto* ctrl : buttons) {
        // Button extra flags: ClipImage, WrapText
        int btn_flags = 0;
        if (ctrl->clip_image) btn_flags |= kGUICtrl_Clip;
        if (ctrl->wrap_text) btn_flags |= kGUICtrl_WrapText;
        WriteGUIObjectBase(out, *ctrl, kGUIButton, !ctrl->event_handler.empty(), btn_flags);
        // Button-specific data
        out->WriteInt32(ctrl->image);           // pic
        out->WriteInt32(ctrl->mouseover_image); // overpic
        out->WriteInt32(ctrl->pushed_image);    // pushedpic
        out->WriteInt32(ctrl->font);            // font
        out->WriteInt32(ctrl->text_color);      // textcol
        out->WriteInt32(ctrl->click_action);    // leftclick
        out->WriteInt32(0);                     // rightclick
        out->WriteInt32(ctrl->new_mode_number); // lclickdata
        out->WriteInt32(0);                     // rclickdata
        FilePutString(out, ctrl->text);         // text
        out->WriteInt32(ctrl->text_alignment);  // textAlignment
    }

    // Labels
    out->WriteInt32((int32_t)labels.size());
    for (const auto* ctrl : labels) {
        WriteGUIObjectBase(out, *ctrl, kGUILabel, false);
        FilePutString(out, ctrl->text);         // text
        out->WriteInt32(ctrl->font);            // font
        out->WriteInt32(ctrl->text_color);      // textColor
        out->WriteInt32(ctrl->text_alignment);  // textAlignment
    }

    // Inventory Windows
    out->WriteInt32((int32_t)inv_windows.size());
    for (const auto* ctrl : inv_windows) {
        WriteGUIObjectBase(out, *ctrl, kGUIInvWindow, false);
        out->WriteInt32(ctrl->inv_char_id);     // charID (-1 = current player)
        out->WriteInt32(ctrl->item_width);      // itemWidth
        out->WriteInt32(ctrl->item_height);     // itemHeight
    }

    // Sliders
    out->WriteInt32((int32_t)sliders.size());
    for (const auto* ctrl : sliders) {
        WriteGUIObjectBase(out, *ctrl, kGUISlider, !ctrl->event_handler.empty());
        out->WriteInt32(ctrl->min_value);       // minValue
        out->WriteInt32(ctrl->max_value);       // maxValue
        out->WriteInt32(ctrl->slider_value);    // value
        out->WriteInt32(ctrl->handle_image);    // handleImage
        out->WriteInt32(ctrl->handle_offset);   // handleOffset
        out->WriteInt32(ctrl->bg_image);        // bgImage
    }

    // TextBoxes
    out->WriteInt32((int32_t)textboxes.size());
    for (const auto* ctrl : textboxes) {
        WriteGUIObjectBase(out, *ctrl, kGUITextBox, !ctrl->event_handler.empty());
        FilePutString(out, ctrl->text);         // text
        out->WriteInt32(ctrl->font);            // font
        out->WriteInt32(ctrl->text_color);      // textColor
        int tb_flags = ctrl->show_border ? kTextBox_ShowBorder : 0;
        out->WriteInt32(tb_flags);              // textBoxFlags
    }

    // ListBoxes
    out->WriteInt32((int32_t)listboxes.size());
    for (const auto* ctrl : listboxes) {
        WriteGUIObjectBase(out, *ctrl, kGUIListBox, !ctrl->event_handler.empty());
        out->WriteInt32(0);                          // numItems (always 0 from editor)
        out->WriteInt32(ctrl->font);                 // font
        out->WriteInt32(ctrl->text_color);            // textColor
        out->WriteInt32(ctrl->selected_text_color);   // selectedTextColor
        int lb_flags = 0;
        if (ctrl->show_border) lb_flags |= kListBox_ShowBorder;
        if (ctrl->show_scroll_arrows) lb_flags |= kListBox_ShowArrows;
        out->WriteInt32(lb_flags);                    // listBoxFlags
        out->WriteInt32(ctrl->text_alignment);        // textAlignment
        out->WriteInt32(ctrl->selected_bg_color);     // selectedBgColor
    }
}

// -------------------------------------------------------------------------
// Section: Plugins
// -------------------------------------------------------------------------
static void WritePlugins(Stream* out, const GameData& gd)
{
    out->WriteInt32(1); // version
    out->WriteInt32(0); // count = 0 (no plugins)
}

// -------------------------------------------------------------------------
// Section: Custom Properties
// -------------------------------------------------------------------------
static void WriteCustomProperties(Stream* out, const GameData& gd)
{
    // Property schema
    out->WriteInt32(kPropertyVersion_Current); // version
    out->WriteInt32((int32_t)gd.custom_property_schemas.size());
    for (const auto& prop : gd.custom_property_schemas) {
        FilePutString(out, prop.name);
        // Map type: our 0=bool,1=int,2=String to engine 1=bool,2=int,3=String
        out->WriteInt32(prop.type + 1);
        FilePutString(out, prop.description);
        FilePutString(out, prop.default_value);
    }

    // Per-character properties
    for (size_t i = 0; i < gd.characters.size(); i++) {
        out->WriteInt32(kPropertyVersion_Current);
        out->WriteInt32((int32_t)gd.characters[i].custom_properties.size());
        for (const auto& kv : gd.characters[i].custom_properties) {
            FilePutString(out, kv.first);
            FilePutString(out, kv.second);
        }
    }

    // Per-inventory properties (slot 0 dummy + real items)
    // Slot 0
    out->WriteInt32(1); // version 1 for dummy
    out->WriteInt32(0); // no properties
    // Real items
    for (size_t i = 0; i < gd.inventory_items.size(); i++) {
        out->WriteInt32(kPropertyVersion_Current);
        out->WriteInt32((int32_t)gd.inventory_items[i].custom_properties.size());
        for (const auto& kv : gd.inventory_items[i].custom_properties) {
            FilePutString(out, kv.first);
            FilePutString(out, kv.second);
        }
    }
}

// -------------------------------------------------------------------------
// Section: Object Names (View names + Inventory names)
// -------------------------------------------------------------------------
static void WriteObjectNames(Stream* out, const GameData& gd)
{
    // View names
    for (const auto& v : gd.views)
        WriteNulTermString(out, v.name);

    // Inventory names — slot 0 = empty, then real items
    out->WriteInt8(0); // slot 0 empty name
    for (const auto& inv : gd.inventory_items)
        WriteNulTermString(out, inv.script_name);
}

// -------------------------------------------------------------------------
// Section: Audio Clip Types
// -------------------------------------------------------------------------
static void WriteAudioClipTypes(Stream* out, const GameData& gd)
{
    out->WriteInt32((int32_t)gd.audio_clip_types.size());
    for (const auto& t : gd.audio_clip_types) {
        out->WriteInt32(t.id);
        out->WriteInt32(t.max_channels);
        out->WriteInt32(t.volume_reduction_while_speech);
        out->WriteInt32(t.crossfade_speed);
        out->WriteInt32(0); // reservedForFuture
    }
}

// -------------------------------------------------------------------------
// Section: Audio Clips (64 bytes each)
// -------------------------------------------------------------------------
static void WriteAudioClips(Stream* out, const GameData& gd)
{
    out->WriteInt32((int32_t)gd.audio_clips.size());
    for (const auto& clip : gd.audio_clips) {
        // id (int32)
        out->WriteInt32(clip.id);
        // scriptName[30] — fixed-length
        WriteFixedString(out, clip.name, 30);
        // fileName[15] — fixed-length
        WriteFixedString(out, clip.filename, 15);
        // bundlingType (uint8)
        out->WriteInt8((uint8_t)clip.bundling_type);
        // type (uint8)
        out->WriteInt8((uint8_t)clip.type);
        // fileType (uint8)
        out->WriteInt8((uint8_t)clip.file_type);
        // defaultRepeat (uint8) — 0 or 1
        out->WriteInt8((uint8_t)(clip.default_repeat > 0 ? 1 : 0));
        // padding (uint8)
        out->WriteInt8(0);
        // defaultPriority (int16)
        out->WriteInt16((int16_t)clip.default_priority);
        // defaultVolume (int16)
        out->WriteInt16((int16_t)clip.default_volume);
        // padding (2 bytes)
        out->WriteInt16(0);
        // reserved (int32)
        out->WriteInt32(0);
    }
}

// -------------------------------------------------------------------------
// Section: Room Names (debug only)
// -------------------------------------------------------------------------
static void WriteRoomNames(Stream* out, const GameData& gd)
{
    if (!gd.debug_mode) return;

    out->WriteInt32((int32_t)gd.rooms.size());
    for (const auto& r : gd.rooms) {
        out->WriteInt32(r.number);
        WriteNulTermString(out, r.description);
    }
}

// -------------------------------------------------------------------------
// Extension helpers
// -------------------------------------------------------------------------
static void BeginExtension(Stream* out, const char* ext_id, soff_t& data_start)
{
    out->WriteInt8(0x00); // extension marker
    // Write 16-byte extension ID (zero-padded)
    char id_buf[16] = {};
    strncpy(id_buf, ext_id, 16);
    out->Write(id_buf, 16);
    // Data length placeholder (int64)
    data_start = out->GetPosition();
    out->WriteInt64(0); // placeholder
}

static void EndExtension(Stream* out, soff_t data_start)
{
    soff_t end_pos = out->GetPosition();
    soff_t data_len = end_pos - data_start - 8; // subtract the int64 placeholder itself
    out->Seek(data_start, AGS::Common::kSeekBegin);
    out->WriteInt64(data_len);
    out->Seek(end_pos, AGS::Common::kSeekBegin);
}

// -------------------------------------------------------------------------
// Extension: v360_fonts
// -------------------------------------------------------------------------
static void WriteExt_v360_fonts(Stream* out, const GameData& gd)
{
    soff_t data_start;
    BeginExtension(out, "v360_fonts", data_start);

    for (const auto& f : gd.fonts) {
        // autoOutlineThickness
        out->WriteInt32(f.outline_type == 1 ? 1 : 0); // auto outline thickness
        // autoOutlineStyle — 0=squared, 1=rounded
        out->WriteInt32(0);
        // characterSpacing (since 3.6.3)
        out->WriteInt32(0);
        // customHeightValue (since 3.6.3)
        out->WriteInt32(0);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
    }

    EndExtension(out, data_start);
}

// -------------------------------------------------------------------------
// Extension: v360_cursors
// -------------------------------------------------------------------------
static void WriteExt_v360_cursors(Stream* out, const GameData& gd)
{
    soff_t data_start;
    BeginExtension(out, "v360_cursors", data_start);

    for (const auto& c : gd.cursors) {
        out->WriteInt32(c.animation_delay); // animdelay
        out->WriteInt32(0); // reserved
        out->WriteInt32(0); // reserved
        out->WriteInt32(0); // reserved
    }

    EndExtension(out, data_start);
}

// -------------------------------------------------------------------------
// Extension: v361_objnames
// -------------------------------------------------------------------------
static void WriteExt_v361_objnames(Stream* out, const GameData& gd)
{
    soff_t data_start;
    BeginExtension(out, "v361_objnames", data_start);

    // gamename (full length)
    FilePutString(out, gd.game_title);
    // saveGameFolderName (full length)
    FilePutString(out, gd.save_game_folder);

    // Characters
    out->WriteInt32((int32_t)gd.characters.size());
    for (const auto& ch : gd.characters) {
        FilePutString(out, ch.script_name);
        FilePutString(out, ch.real_name);
    }

    // Inventory (includes dummy slot 0)
    out->WriteInt32((int32_t)(gd.inventory_items.size() + 1));
    out->WriteInt32(0); // slot 0 empty name (len=0)
    for (const auto& inv : gd.inventory_items)
        FilePutString(out, inv.description);

    // Cursors
    out->WriteInt32((int32_t)gd.cursors.size());
    for (const auto& c : gd.cursors)
        FilePutString(out, c.name);

    // Audio clips
    out->WriteInt32((int32_t)gd.audio_clips.size());
    for (const auto& clip : gd.audio_clips) {
        FilePutString(out, clip.name);
        FilePutString(out, clip.filename);
    }

    EndExtension(out, data_start);
}

// -------------------------------------------------------------------------
// Extension: v362_interevent2
// -------------------------------------------------------------------------
static void WriteExt_v362_interevent2(Stream* out, const GameData& gd)
{
    soff_t data_start;
    BeginExtension(out, "v362_interevent2", data_start);

    // Global script filename
    FilePutString(out, std::string("GlobalScript.asc"));
    // Dialog script filename
    FilePutString(out, std::string("DialogScript.asc"));

    // Script module count — must match the number of compiled script modules
    // written in WriteCompiledScripts. This includes _GlobalVariables (if any
    // global variables exist) plus all user modules (everything from
    // gd.script_modules except GlobalScript).
    bool has_global_vars = !gd.global_variables.empty();
    int module_count = 0;
    for (size_t i = 0; i < gd.script_modules.size(); i++) {
        if (gd.script_modules[i].name == "GlobalScript") continue;
        module_count++;
    }
    if (has_global_vars)
        module_count++; // _GlobalVariables is an extra module
    out->WriteInt32(module_count);
    // _GlobalVariables comes first (matching C# editor order)
    if (has_global_vars)
        FilePutString(out, std::string("_GlobalVariables.asc"));
    for (size_t i = 0; i < gd.script_modules.size(); i++) {
        if (gd.script_modules[i].name == "GlobalScript") continue;
        FilePutString(out, gd.script_modules[i].script_file);
    }

    // Characters — interaction events
    out->WriteInt32((int32_t)gd.characters.size());
    for (const auto& ch : gd.characters) {
        const int v362_version = 3060200;
        out->WriteInt32(v362_version);
        FilePutString(out, ch.interactions.script_module);
        out->WriteInt32((int32_t)ch.interactions.handler_functions.size());
        for (const auto& h : ch.interactions.handler_functions)
            FilePutString(out, h);
    }

    // Inventory items (slot 0 dummy + real items)
    out->WriteInt32((int32_t)(gd.inventory_items.size() + 1));
    // Slot 0 dummy
    {
        const int v362_version = 3060200;
        out->WriteInt32(v362_version);
        FilePutString(out, std::string(""));
        out->WriteInt32(0);
    }
    for (const auto& inv : gd.inventory_items) {
        const int v362_version = 3060200;
        out->WriteInt32(v362_version);
        FilePutString(out, inv.interactions.script_module);
        out->WriteInt32((int32_t)inv.interactions.handler_functions.size());
        for (const auto& h : inv.interactions.handler_functions)
            FilePutString(out, h);
    }

    // GUIs — script module name for each GUI's events
    out->WriteInt32((int32_t)gd.guis.size());
    for (const auto& gui : gd.guis)
        FilePutString(out, gui.script_module);

    EndExtension(out, data_start);
}

// -------------------------------------------------------------------------
// Extension: v363_gameinfo
// -------------------------------------------------------------------------
static void WriteExt_v363_gameinfo(Stream* out, const GameData& gd)
{
    soff_t data_start;
    BeginExtension(out, "v363_gameinfo", data_start);

    // Key-value pairs
    std::vector<std::pair<std::string, std::string>> info;
    if (!gd.game_title.empty()) info.push_back({"title", gd.game_title});
    if (!gd.description.empty()) info.push_back({"description", gd.description});
    if (!gd.developer_name.empty()) info.push_back({"dev_name", gd.developer_name});
    if (!gd.developer_url.empty()) info.push_back({"dev_url", gd.developer_url});

    out->WriteInt32((int32_t)info.size());
    for (const auto& kv : info) {
        FilePutString(out, kv.first);
        FilePutString(out, kv.second);
    }

    EndExtension(out, data_start);
}

// -------------------------------------------------------------------------
// Extension: v363_dialogsnew
// -------------------------------------------------------------------------
static void WriteExt_v363_dialogsnew(Stream* out, const GameData& gd)
{
    soff_t data_start;
    BeginExtension(out, "v363_dialogsnew", data_start);

    out->WriteInt32((int32_t)gd.dialogs.size());
    for (const auto& dlg : gd.dialogs) {
        // scriptName
        FilePutString(out, dlg.script_name);
        // topicFlags
        int32_t topic_flags = 0;
        if (dlg.show_text_parser) topic_flags |= DTFLG_SHOWPARSER;
        out->WriteInt32(topic_flags);
        // reserved × 3
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        // option_count
        out->WriteInt32((int32_t)dlg.options.size());

        for (const auto& opt : dlg.options) {
            // text
            FilePutString(out, opt.text);
            // flags
            int32_t opt_flags = 0;
            if (opt.show) opt_flags |= DFLG_ON;
            if (!opt.say) opt_flags |= DFLG_NOREPEAT;
            out->WriteInt32(opt_flags);
            // reserved × 3
            out->WriteInt32(0);
            out->WriteInt32(0);
            out->WriteInt32(0);
        }
    }

    EndExtension(out, data_start);
}

// -------------------------------------------------------------------------
// Extension: v363_guictrls2
// -------------------------------------------------------------------------
static void WriteCommonControlProps(Stream* out)
{
    // backgroundColor, borderColor, borderWidth, paddingX, paddingY, reserved×4
    out->WriteInt32(0); // backgroundColor
    out->WriteInt32(0); // borderColor
    out->WriteInt32(0); // borderWidth
    out->WriteInt32(0); // paddingX
    out->WriteInt32(0); // paddingY
    out->WriteInt32(0); // reserved
    out->WriteInt32(0); // reserved
    out->WriteInt32(0); // reserved
    out->WriteInt32(0); // reserved
}

static void WriteExt_v363_guictrls2(Stream* out, const GameData& gd)
{
    // Collect controls by type (same order as in WriteGUIs)
    std::vector<const GameData::GUIInfo::ControlInfo*> buttons;
    std::vector<const GameData::GUIInfo::ControlInfo*> labels;
    std::vector<const GameData::GUIInfo::ControlInfo*> inv_windows;
    std::vector<const GameData::GUIInfo::ControlInfo*> sliders_vec;
    std::vector<const GameData::GUIInfo::ControlInfo*> textboxes;
    std::vector<const GameData::GUIInfo::ControlInfo*> listboxes;

    for (const auto& gui : gd.guis) {
        for (const auto& ctrl : gui.controls) {
            int type_id = ControlTypeFromTag(ctrl.type_tag);
            switch (type_id) {
                case kGUIButton: buttons.push_back(&ctrl); break;
                case kGUILabel: labels.push_back(&ctrl); break;
                case kGUIInvWindow: inv_windows.push_back(&ctrl); break;
                case kGUISlider: sliders_vec.push_back(&ctrl); break;
                case kGUITextBox: textboxes.push_back(&ctrl); break;
                case kGUIListBox: listboxes.push_back(&ctrl); break;
            }
        }
    }

    soff_t data_start;
    BeginExtension(out, "v363_guictrls2", data_start);

    // Buttons
    out->WriteInt32((int32_t)buttons.size());
    for (size_t i = 0; i < buttons.size(); i++) {
        WriteCommonControlProps(out);
        // buttonFlags, borderShadeColor, mouseOverBgColor, pushedBgColor,
        // mouseOverBorderColor, pushedBorderColor, mouseOverTextColor,
        // pushedTextColor, textOutlineColor, reserved×3
        for (int j = 0; j < 12; j++) out->WriteInt32(0);
    }

    // Labels
    out->WriteInt32((int32_t)labels.size());
    for (size_t i = 0; i < labels.size(); i++) {
        WriteCommonControlProps(out);
        // textOutlineColor, reserved×3
        for (int j = 0; j < 4; j++) out->WriteInt32(0);
    }

    // Inventory Windows
    out->WriteInt32((int32_t)inv_windows.size());
    for (size_t i = 0; i < inv_windows.size(); i++) {
        WriteCommonControlProps(out);
        // reserved×4
        for (int j = 0; j < 4; j++) out->WriteInt32(0);
    }

    // Sliders
    out->WriteInt32((int32_t)sliders_vec.size());
    for (size_t i = 0; i < sliders_vec.size(); i++) {
        WriteCommonControlProps(out);
        // handleColor, borderShadeColor, reserved×4
        for (int j = 0; j < 6; j++) out->WriteInt32(0);
    }

    // TextBoxes
    out->WriteInt32((int32_t)textboxes.size());
    for (size_t i = 0; i < textboxes.size(); i++) {
        WriteCommonControlProps(out);
        // textAlignment, textOutlineColor, reserved×2
        for (int j = 0; j < 4; j++) out->WriteInt32(0);
    }

    // ListBoxes
    out->WriteInt32((int32_t)listboxes.size());
    for (size_t i = 0; i < listboxes.size(); i++) {
        WriteCommonControlProps(out);
        // textOutlineColor, reserved×3
        for (int j = 0; j < 4; j++) out->WriteInt32(0);
    }

    EndExtension(out, data_start);
}

// -------------------------------------------------------------------------
// Main writer function
// -------------------------------------------------------------------------
bool WriteGameDataFile(const GameData& gd,
                       const std::vector<CompiledScriptRef>& compiled_scripts,
                       const std::string& output_path,
                       BuildResult& result)
{
    std::unique_ptr<Stream> out(
        AGS::Common::File::CreateFile(output_path.c_str()));
    if (!out) {
        result.AddError("", 0, "Cannot create game data file: " + output_path);
        return false;
    }

    // ---- File Header ----
    out->Write(GAME_FILE_SIG, 30);
    out->WriteInt32(GAME_DATA_VERSION_CURRENT);
    // Engine version string (length-prefixed)
    int32_t ver_len = (int32_t)strlen(ENGINE_VERSION);
    out->WriteInt32(ver_len);
    out->Write(ENGINE_VERSION, ver_len);
    // Extended engine capabilities count
    out->WriteInt32(0);

    // ---- GameSetupStructBase ----
    soff_t ext_off_pos = 0;
    WriteGameSetupStructBase(out.get(), gd, ext_off_pos);

    // ---- Save Game Info ----
    WriteSaveGameInfo(out.get(), gd);

    // ---- Font Infos ----
    WriteFontInfos(out.get(), gd);

    // ---- Sprite Flags ----
    WriteSpriteFlags(out.get(), gd);

    // ---- Inventory Items ----
    WriteInventoryItems(out.get(), gd);

    // ---- Mouse Cursors ----
    WriteMouseCursors(out.get(), gd);

    // ---- Interaction Scripts (legacy) ----
    WriteInteractionScripts(out.get(), gd);

    // ---- Words Dictionary ----
    WriteWordsDictionary(out.get(), gd);

    // ---- Compiled Scripts ----
    if (!WriteCompiledScripts(out.get(), gd, compiled_scripts, result))
        return false;

    // ---- Views ----
    WriteViews(out.get(), gd);

    // ---- Characters ----
    WriteCharacters(out.get(), gd);

    // ---- Lip-Sync Data ----
    WriteLipSyncData(out.get(), gd);

    // ---- Global Messages ----
    WriteGlobalMessages(out.get(), gd);

    // ---- GUIs ----
    WriteGUIs(out.get(), gd);

    // ---- Plugins ----
    WritePlugins(out.get(), gd);

    // ---- Custom Properties ----
    WriteCustomProperties(out.get(), gd);

    // ---- Object Names ----
    WriteObjectNames(out.get(), gd);

    // ---- Audio Clip Types ----
    WriteAudioClipTypes(out.get(), gd);

    // ---- Audio Clips ----
    WriteAudioClips(out.get(), gd);

    // ---- Score Sound Clip ID ----
    out->WriteInt32(gd.score_sound_clip);

    // ---- Room Names (debug only) ----
    WriteRoomNames(out.get(), gd);

    // ---- Extensions ----
    // Backpatch the ext_off
    soff_t ext_start = out->GetPosition();
    out->Seek(ext_off_pos, AGS::Common::kSeekBegin);
    out->WriteInt32((uint32_t)ext_start);
    out->Seek(ext_start, AGS::Common::kSeekBegin);

    WriteExt_v360_fonts(out.get(), gd);
    WriteExt_v360_cursors(out.get(), gd);
    WriteExt_v361_objnames(out.get(), gd);
    WriteExt_v362_interevent2(out.get(), gd);
    WriteExt_v363_gameinfo(out.get(), gd);
    WriteExt_v363_dialogsnew(out.get(), gd);
    WriteExt_v363_guictrls2(out.get(), gd);

    // End-of-extensions marker
    out->WriteInt8((uint8_t)0xFF);

    return true;
}

} // namespace AGSEditor
