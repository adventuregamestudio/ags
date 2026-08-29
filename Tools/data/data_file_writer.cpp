//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "data_file_writer.h"

// This implements the current game28.dta layout used by DataFileWriter.cs,
// including parser words, views and characters, sprite/font data, lip-sync and
// global messages, plugins and custom properties, audio metadata, palette and
// game options, GUI appearance data, and the complete extension list.
//
// Deliberate differences from the Editor writer:
//
//  * compiled global/dialog scripts and script modules are not embedded. The
//    legacy HasCCScript flag is clear, because AGS 3.6.1+ may load these from
//    the separately generated script files;
//  * v362_interevent2 retains character and inventory event function names,
//    but its embedded global/dialog/module filenames are empty until those
//    external-file names are part of GameData;
//  * settings that are absent from Game.agf/GameData (notably the legacy
//    letterbox-resolution value) still use the modern/default representation.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ac/characterinfo.h"
#include "ac/dialogtopic.h"
#include "ac/view.h"
#include "ac/wordsdictionary.h"
#include "ac/dynobj/scriptaudioclip.h"
#include "ac/game_version.h"
#include "ac/gamesetupstructbase.h"
#include "ac/inventoryiteminfo.h"
#include "ac/mousecursor.h"
#include "game/customproperties.h"
#include "game/interactions.h"
#include "gfx/gfx_def.h"
#include "gui/guibutton.h"
#include "gui/guidefines.h"
#include "util/string_utils.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataFileWriter
{

bool ParseResolution(const String &text, int &width, int &height)
{
    width = 320;
    height = 200;
    if (text.IsEmpty())
        return false;

    int parsed_width = 0;
    int parsed_height = 0;
    if (std::sscanf(text.GetCStr(), "%d x %d", &parsed_width, &parsed_height) == 2 ||
        std::sscanf(text.GetCStr(), "%dx%d", &parsed_width, &parsed_height) == 2 ||
        std::sscanf(text.GetCStr(), "%d,%d", &parsed_width, &parsed_height) == 2)
    {
        if (parsed_width > 0 && parsed_height > 0)
        {
            width = parsed_width;
            height = parsed_height;
            return true;
        }
    }
    return false;
}

// Read by Properties::ReadSchema() in Common/game/customproperties.cpp,
// called from GameSetupStruct::read_customprops().
void WritePropertySchemaBlock(Stream *out,
    const std::vector<DataUtil::CustomPropertySchemaItem> &schema)
{
    out->WriteInt32(AGS::Common::kPropertyVersion_Current);
    out->WriteInt32(static_cast<int32_t>(schema.size()));
    for (const auto &item : schema)
    {
        StrUtil::WriteString(item.Name, out);
        out->WriteInt32(static_cast<int32_t>(item.Type));
        StrUtil::WriteString(item.Description, out);
        StrUtil::WriteString(item.DefaultValue, out);
    }
}

// Read by Properties::ReadValues() in Common/game/customproperties.cpp for
// inventory slot 0, from GameSetupStruct::read_customprops().
void WriteEmptyPropertyValues(Stream *out)
{
    out->WriteInt32(AGS::Common::kPropertyVersion_Current);
    out->WriteInt32(0); // number of items
}

// Read inline by GameSetupStruct::read_font_infos() in
// Common/ac/gamesetupstruct.cpp.
void WriteFontInfo(Stream *out, const DataUtil::FontData &font)
{
    int flags = 0;
    if (font.PointSize == 0) flags |= FFLG_SIZEMULTIPLIER;
    if (font.HeightDefinedBy == DataUtil::kFontHeight_NominalHeight) flags |= FFLG_LOGICALNOMINALHEIGHT;
    if (font.HeightDefinedBy == DataUtil::kFontHeight_CustomValue) flags |= FFLG_LOGICALCUSTOMHEIGHT;
    if (font.MetricsFixup == DataUtil::kFontMetrics_SetAscenderToHeight) flags |= FFLG_ASCENDERFIXUP;
    out->WriteInt32(flags);
    out->WriteInt32(font.PointSize == 0 ? font.SizeMultiplier : font.PointSize * font.SizeMultiplier);
    out->WriteInt32(font.OutlineStyle == DataUtil::kFontOutline_Automatic ? FONT_OUTLINE_AUTO :
        font.OutlineStyle == DataUtil::kFontOutline_UseOutlineFont ? font.OutlineFont : FONT_OUTLINE_NONE);
    out->WriteInt32(font.VerticalOffset);
    out->WriteInt32(font.LineSpacing);
}

// Read by Properties::ReadValues() in Common/game/customproperties.cpp,
// called from GameSetupStruct::read_customprops().
void WritePropertyValues(Stream *out,
    const std::vector<DataUtil::CustomPropertyValue> &properties)
{
    out->WriteInt32(AGS::Common::kPropertyVersion_Current);
    out->WriteInt32(static_cast<int32_t>(properties.size()));
    for (const auto &property : properties)
    {
        StrUtil::WriteString(property.Name, out);
        StrUtil::WriteString(property.Value, out);
    }
}

// Read by InteractionEvents::CreateFromStream_v361() in
// Common/game/interactions.cpp, called from read_interaction_scripts().
void WriteEmptyInteractionEvents(Stream *out)
{
    out->WriteInt32(0);
}

int GetAudioID(const DataUtil::GameData &game, int fixed_index)
{
    for (const auto &clip : game.AudioClips)
        if (clip.Index == fixed_index) return clip.ID;
    return -1;
}

// Read by ViewStruct::ReadFromFile() in Common/ac/view.cpp, called from
// ReadViews() in Common/game/main_game_file.cpp.
void WriteView(Stream *out, const DataUtil::GameData &game, const DataUtil::ViewData &view)
{
    out->WriteInt16(static_cast<int16_t>(view.Loops.size()));
    for (const auto &loop : view.Loops)
    {
        out->WriteInt16(static_cast<int16_t>(loop.Frames.size()));
        out->WriteInt32(loop.RunNextLoop ? LOOPFLAG_RUNNEXTLOOP : 0);
        for (const auto &frame : loop.Frames)
        {
            out->WriteInt32(frame.Image);
            out->WriteInt16(0);
            out->WriteInt16(0);
            out->WriteInt16(static_cast<int16_t>(frame.Delay));
            out->WriteInt16(0);
            out->WriteInt32(frame.Flipped ? VFLG_FLIPSPRITE : 0);
            out->WriteInt32(GetAudioID(game, frame.Sound));
            out->WriteInt32(0);
            out->WriteInt32(0);
        }
    }
}

// Read by CharacterInfo::ReadFromFile() in Common/ac/characterinfo.cpp,
// through GameSetupStruct::ReadCharacters().
void WriteCharacter(Stream *out, const DataUtil::GameData &game,
    const DataUtil::CharacterData &ref, int index)
{
    CharacterInfo chinfo;
    CharacterInfo2 chinfo2;

    chinfo.index_id = ref.ID >= 0 ? ref.ID : index;
    chinfo.on = 1;
    chinfo.defview = ref.NormalView - 1;
    chinfo.talkview = ref.SpeechView - 1;
    chinfo.view = ref.NormalView - 1;
    chinfo.room = ref.StartingRoom;
    chinfo.x = ref.StartX;
    chinfo.y = ref.StartY;
    chinfo.idleview = ref.IdleView - 1;
    chinfo.idledelay = static_cast<int16_t>(ref.IdleDelay);
    chinfo.transparency = static_cast<int16_t>(GfxDef::Trans100ToLegacyTrans255(ref.Transparency));
    chinfo.baseline = static_cast<int16_t>(ref.Baseline);
    chinfo.talkcolor = ref.SpeechColor;
    chinfo.thinkview = ref.ThinkingView - 1;
    chinfo.blinkview = static_cast<int16_t>(ref.BlinkingView - 1);
    chinfo.walkspeed_y = static_cast<int16_t>(ref.UniformMovementSpeed ? UNIFORM_WALK_SPEED : ref.MovementSpeedY);
    chinfo.speech_anim_speed = static_cast<int16_t>(ref.SpeechAnimationDelay);
    chinfo.idle_anim_speed = static_cast<int16_t>(ref.IdleAnimationDelay);
    chinfo.blocking_width = static_cast<int16_t>(ref.BlockingWidth);
    chinfo.blocking_height = static_cast<int16_t>(ref.BlockingHeight);
    chinfo.walkspeed = static_cast<int16_t>(ref.UniformMovementSpeed ? ref.MovementSpeed : ref.MovementSpeedX);
    chinfo.animspeed = static_cast<int16_t>(ref.AnimationDelay);
    if (ref.AdjustSpeedWithScaling) chinfo.flags |= CHF_SCALEMOVESPEED;
    if (ref.AdjustVolumeWithScaling) chinfo.flags |= CHF_SCALEVOLUME;
    if (!ref.Clickable) chinfo.flags |= CHF_NOINTERACT;
    if (!ref.DiagonalLoops) chinfo.flags |= CHF_NODIAGONAL;
    if (ref.MovementLinkedToAnimation) chinfo.flags |= CHF_ANTIGLIDE;
    if (!ref.Solid) chinfo.flags |= CHF_NOBLOCKING;
    if (!ref.TurnBeforeWalking) chinfo.flags |= CHF_NOTURNWHENWALK;
    if (ref.TurnWhenFacing) chinfo.flags |= CHF_TURNWHENFACE;
    if (!ref.UseRoomAreaLighting) chinfo.flags |= CHF_NOLIGHTING;
    if (!ref.UseRoomAreaScaling) chinfo.flags |= CHF_MANUALSCALING;
    if ((ref.ID >= 0 ? ref.ID : index) == game.PlayerCharacter)
        for (const auto &item : game.Inventory)
            if (item.PlayerStartsWith && item.ID >= 0 && item.ID < MAX_INV) chinfo.inv[item.ID] = 1;
    chinfo2.blocking_x = ref.BlockingX;
    chinfo2.blocking_y = ref.BlockingY;

    const String &name = ref.RealName;
    std::snprintf(chinfo.name, LEGACY_MAX_CHAR_NAME_LEN, "%s", name.GetCStr());
    std::snprintf(chinfo.scrname, LEGACY_MAX_SCRIPT_NAME_LEN, "%s", ref.ScriptName.GetCStr());
    chinfo2.name_new = name;
    chinfo2.scrname_new = ref.ScriptName;

    chinfo.WriteToFile(chinfo2, out);
}

// Read by InventoryItemInfo::ReadFromFile() in
// Common/ac/inventoryiteminfo.cpp.
void WriteInventoryItem(Stream *out, const DataUtil::InventoryItemData &ref)
{
    InventoryItemInfo item{};
    item.name = ref.Description;
    item.pic = ref.Image;
    item.cursorPic = ref.CursorImage;
    item.hotx = ref.HotspotX;
    item.hoty = ref.HotspotY;
    item.flags = ref.PlayerStartsWith ? IFLG_STARTWITH : 0;
    item.WriteToFile(out);
}

// Read by MouseCursor::ReadFromFile() in Common/ac/mousecursor.cpp.
void WriteCursor(Stream *out, const DataUtil::CursorData &ref)
{
    MouseCursor cur;
    const String &name = ref.Name;
    std::snprintf(cur.legacy_name, LEGACY_MAX_CURSOR_NAME_LENGTH, "%s", name.GetCStr());
    cur.name = name;
    cur.pic = ref.Image;
    cur.hotx = static_cast<short>(ref.HotspotX);
    cur.hoty = static_cast<short>(ref.HotspotY);
    cur.view = static_cast<short>(ref.Animate && ref.View > 0 ? ref.View - 1 : -1);
    cur.flags = 0;
    if (ref.Animate && ref.AnimateOnlyOnHotspots) cur.flags |= MCF_HOTSPOT;
    if (ref.Animate && ref.AnimateOnlyWhenMoving) cur.flags |= MCF_ANIMMOVE;
    if (ref.StandardMode) cur.flags |= MCF_STANDARD;
    cur.WriteToFile(out);
}

int GetGuiPopupStyle(DataUtil::GUIPopupStyle style)
{
    switch (style)
    {
    case DataUtil::kGUIPopupStyle_MouseYPos: return kGUIPopupMouseY;
    case DataUtil::kGUIPopupStyle_PopupModal: return kGUIPopupModal;
    case DataUtil::kGUIPopupStyle_Persistent: return kGUIPopupNoAutoRemove;
    case DataUtil::kGUIPopupStyle_Normal:
    default: return kGUIPopupNormal;
    }
}

// GUI classes in Common have equivalent WriteToFile methods, but using them here
// would pull their runtime implementations into libtools and dependencies.
// New GUIObjects also cannot initialize serialized event-handler slots through
// the public API, and v363_guictrls2 has no writer there.
// Keep this until Common is refactored to include a data-only GUI writer.
// Read by GUIMain::ReadFromFile() in Common/gui/guimain.cpp, called from
// GUI::ReadGUI().
void WriteGui(Stream *out, const DataUtil::GUIData &ref,
    const std::vector<uint32_t> &control_refs)
{
    const String &name = ref.ScriptName.IsEmpty() ? ref.TypeName : ref.ScriptName;
    StrUtil::WriteString(name, out);
    StrUtil::WriteString(ref.IsTextWindow ? String() : ref.OnClick, out);
    out->WriteInt32(ref.IsTextWindow ? 0 : ref.Left);
    out->WriteInt32(ref.IsTextWindow ? 0 : ref.Top);
    out->WriteInt32(ref.IsTextWindow ? 200 : ref.Width);
    out->WriteInt32(ref.IsTextWindow ? 100 : ref.Height);
    out->WriteInt32(static_cast<int32_t>(control_refs.size()));
    out->WriteInt32(ref.IsTextWindow ? kGUIPopupModal : GetGuiPopupStyle(ref.PopupStyle));
    out->WriteInt32(ref.IsTextWindow ? -1 : ref.PopupYPos);
    out->WriteInt32(ref.BackgroundColor);
    out->WriteInt32(ref.BackgroundImage);
    out->WriteInt32(ref.IsTextWindow ? ref.TextColor : ref.BorderColor);
    int flags = ref.IsTextWindow ? kGUIMain_TextWindow : kGUIMain_DefFlags;
    if (!ref.IsTextWindow && !ref.Clickable) flags &= ~kGUIMain_Clickable;
    if (!ref.IsTextWindow && !ref.Visible) flags &= ~kGUIMain_Visible;
    out->WriteInt32(flags);
    out->WriteInt32(ref.IsTextWindow ? 0 : GfxDef::Trans100ToLegacyTrans255(ref.Transparency));
    out->WriteInt32(ref.IsTextWindow ? -1 : ref.ZOrder);
    out->WriteInt32(0); // legacy guiId field is unused
    out->WriteInt32(ref.IsTextWindow ? ref.Padding : TEXTWINDOW_PADDING_DEFAULT);
    for (uint32_t control_ref : control_refs)
        out->WriteInt32(control_ref);
}

int GetGuiControlFlags(const DataUtil::GUIControlData &control)
{
    int flags = 0;
    if (control.Clickable) flags |= kGUICtrl_Clickable;
    if (control.Enabled) flags |= kGUICtrl_Enabled;
    if (control.Visible) flags |= kGUICtrl_Visible;
    if (control.Translated) flags |= kGUICtrl_Translated;
    if (control.ShowBorder) flags |= kGUICtrl_ShowBorder;
    if (control.SolidBackground) flags |= kGUICtrl_SolidBack;
    return flags;
}

// Read by GUIObject::ReadFromFile() in Common/gui/guiobject.cpp as the common
// prefix of each concrete GUI control's ReadFromFile().
void WriteGuiControl(Stream *out, const DataUtil::GUIControlData &control,
    int extra_flags, const String *event_handler)
{
    out->WriteInt32(GetGuiControlFlags(control) | extra_flags);
    out->WriteInt32(control.Left);
    out->WriteInt32(control.Top);
    out->WriteInt32(control.Width);
    out->WriteInt32(control.Height);
    out->WriteInt32(control.ZOrder);
    StrUtil::WriteCStr(control.ScriptName, out);
    out->WriteInt32(event_handler ? 1 : 0);
    if (event_handler)
        StrUtil::WriteCStr(*event_handler, out);
}

int GetButtonClickAction(const String &action)
{
    if (action.CompareNoCase("SetCursorMode") == 0 || action.CompareNoCase("SetMode") == 0)
        return kGUIAction_SetMode;
    if (action.CompareNoCase("RunScript") == 0)
        return kGUIAction_RunScript;
    return 0;
}

// Read by GUIButton::ReadFromFile() in Common/gui/guibutton.cpp.
void WriteGuiButton(Stream *out, const DataUtil::GUIButtonData &button)
{
    const int flags = (button.ClipImage ? kGUICtrl_Clip : 0) |
        (button.WrapText ? kGUICtrl_WrapText : 0);
    WriteGuiControl(out, button, flags, &button.OnClick);
    out->WriteInt32(button.Image);
    out->WriteInt32(button.MouseoverImage);
    out->WriteInt32(button.PushedImage);
    out->WriteInt32(button.Font);
    out->WriteInt32(button.TextColor);
    out->WriteInt32(GetButtonClickAction(button.ClickAction));
    out->WriteInt32(0);
    out->WriteInt32(button.NewModeNumber);
    out->WriteInt32(0);
    StrUtil::WriteString(button.Text, out);
    out->WriteInt32(button.TextAlignment);
}

// Read by GUILabel::ReadFromFile() in Common/gui/guilabel.cpp.
void WriteGuiLabel(Stream *out, const DataUtil::GUILabelData &label)
{
    WriteGuiControl(out, label, 0, nullptr);
    StrUtil::WriteString(label.Text, out);
    out->WriteInt32(label.Font);
    out->WriteInt32(label.TextColor);
    out->WriteInt32(label.TextAlignment);
}

// Read by GUIInvWindow::ReadFromFile() in Common/gui/guiinv.cpp.
void WriteGuiInvWindow(Stream *out, const DataUtil::GUIInventoryData &inv)
{
    WriteGuiControl(out, inv, 0, nullptr);
    out->WriteInt32(inv.CharacterID);
    out->WriteInt32(inv.ItemWidth);
    out->WriteInt32(inv.ItemHeight);
}

// Read by GUISlider::ReadFromFile() in Common/gui/guislider.cpp.
void WriteGuiSlider(Stream *out, const DataUtil::GUISliderData &slider)
{
    WriteGuiControl(out, slider, 0, &slider.OnChange);
    out->WriteInt32(slider.MinValue);
    out->WriteInt32(slider.MaxValue);
    out->WriteInt32(slider.Value);
    out->WriteInt32(slider.HandleImage);
    out->WriteInt32(slider.HandleOffset);
    out->WriteInt32(slider.BackgroundImage);
}

// Read by GUITextBox::ReadFromFile() in Common/gui/guitextbox.cpp.
void WriteGuiTextBox(Stream *out, const DataUtil::GUITextBoxData &text_box)
{
    WriteGuiControl(out, text_box, text_box.ShowBorder ? kGUICtrl_ShowBorder : 0, &text_box.OnActivate);
    StrUtil::WriteString(text_box.Text, out);
    out->WriteInt32(text_box.Font);
    out->WriteInt32(text_box.TextColor);
    out->WriteInt32(text_box.ShowBorder ? kTextBox_ShowBorder : 0);
}

// Read by GUIListBox::ReadFromFile() in Common/gui/guilistbox.cpp.
void WriteGuiListBox(Stream *out, const DataUtil::GUIListBoxData &list_box)
{
    WriteGuiControl(out, list_box, list_box.ShowBorder ? kGUICtrl_ShowBorder : 0,
        &list_box.OnSelectionChanged);
    out->WriteInt32(0); // runtime items are not part of the project data
    out->WriteInt32(list_box.Font);
    out->WriteInt32(list_box.TextColor);
    out->WriteInt32(list_box.SelectedTextColor);
    int flags = 0;
    if (list_box.ShowBorder) flags |= kListBox_ShowBorder;
    if (list_box.ShowScrollArrows) flags |= kListBox_ShowArrows;
    out->WriteInt32(flags);
    out->WriteInt32(list_box.TextAlignment);
    out->WriteInt32(list_box.SelectedBackgroundColor);
}

// Read by AudioClipType::ReadFromFile() in Common/ac/audiocliptype.cpp.
void WriteAudioType(Stream *out, const DataUtil::AudioTypeData &type, int id)
{
    out->WriteInt32(id);
    out->WriteInt32(type.MaxChannels);
    out->WriteInt32(type.VolumeReductionWhileSpeechPlaying);
    out->WriteInt32(type.Crossfade);
    out->WriteInt32(0);
}

// Read by ScriptAudioClip::ReadFromFile() in
// Common/ac/dynobj/scriptaudioclip.cpp.
void WriteAudioClip(Stream *out, const DataUtil::AudioClipData &ref, int index)
{
    const String &name = ref.ScriptName.IsEmpty() ? ref.TypeName : ref.ScriptName;
    out->WriteInt32(ref.ID >= 0 ? ref.ID : index);
    String script_name = name;
    if (script_name.GetLength() > 29)
        script_name = script_name.Left(29);
    String file_name = ref.CacheFileName;
    StrUtil::WriteFixedString(script_name, 30, out);
    StrUtil::WriteFixedString(file_name, 15, out);
    out->WriteInt8(ref.BundlingType);
    out->WriteInt8(ref.Type);
    out->WriteInt8(ref.FileType);
    out->WriteInt8(ref.Repeat ? 1 : 0);
    out->WriteInt8(0); // alignment padding to int16
    out->WriteInt16(ref.Priority);
    out->WriteInt16(ref.Volume);
    out->WriteInt16(0); // alignment padding to int32
    out->WriteInt32(0); // reserved
}

// Read inline by GameSetupStruct::read_room_names() in
// Common/ac/gamesetupstruct.cpp.
void WriteRoomName(Stream *out, int number, const String &description)
{
    out->WriteInt32(number);
    StrUtil::WriteCStr(description, out);
}

int ParseGameTextEncodingCodePage(const String &value)
{
    if (value.CompareNoCase("UTF-8") == 0 || value.CompareNoCase("utf-8") == 0)
        return 65001;
    if (value.CompareNoCase("ANSI") == 0 || value.CompareNoCase("ansi") == 0)
        return 1252;
    return 65001;
}

int ParseScriptApiVersion(const String &value)
{
    if (value.CompareNoCase("Highest") == 0)
        return kScriptAPI_Current;

    static const CstrArr<14> kScriptApiNames = {{
        "v321", "v330", "v334", "v335", "v340",
        "v341", "v350", "v3507", "v351", "v360",
        "v36026", "v361", "v362", "v363"
    }};
    switch (StrUtil::ParseEnum(value, kScriptApiNames, 13))
    {
    case 0: return kScriptAPI_v321;
    case 1: return kScriptAPI_v330;
    case 2: return kScriptAPI_v334;
    case 3: return kScriptAPI_v335;
    case 4: return kScriptAPI_v340;
    case 5: return kScriptAPI_v341;
    case 6: return kScriptAPI_v350;
    case 7: return kScriptAPI_v3507;
    case 8: return kScriptAPI_v351;
    case 9: return kScriptAPI_v360;
    case 10: return kScriptAPI_v36026;
    case 11: return kScriptAPI_v361;
    case 12: return kScriptAPI_v362;
    case 13:
    default: return kScriptAPI_v363;
    }
}

int ParseSplitResources(const String &value)
{
    return StrUtil::StringToInt(value, 0);
}


std::vector<int32_t> BuildGameOptions(const DataUtil::GameData &game)
{
    // Options without a corresponding GameData field retain their default.
    std::vector<int32_t> options(GameSetupStructBase::MAX_OPTIONS, 0);
    options[OPT_ALWAYSSPCH] = game.Settings.AlwaysDisplayTextAsSpeech ? 1 : 0;
    options[OPT_ANTIALIASFONTS] = game.Settings.AntiAliasFonts ? 1 : 0;
    options[OPT_ANTIGLIDE] = game.Settings.AntiGlideMode ? 1 : 0;
    options[OPT_NOWALKMODE] = game.Settings.AutoMoveInWalkMode ? 0 : 1;
    options[OPT_RIGHTLEFTWRITE] = game.Settings.BackwardsText ? 1 : 0;
    options[OPT_DIALOGUPWARDS] = game.Settings.DialogOptionsBackwards ? 1 : 0;
    options[OPT_DIALOGGAP] = game.Settings.DialogOptionsGap;
    options[OPT_DIALOGIFACE] = game.Settings.DialogOptionsGUI;
    options[OPT_DUPLICATEINV] = game.Settings.DisplayMultipleInventory ? 1 : 0;
    options[OPT_HIRES_FONTS] = 0;
    options[OPT_NEWGUIALPHA] = static_cast<int>(game.Settings.GUIAlphaStyle);
    options[OPT_SPRITEALPHA] = static_cast<int>(game.Settings.SpriteAlphaStyle);
    options[OPT_DIALOGNUMBERED] = static_cast<int>(game.Settings.NumberDialogOptions);
    options[OPT_NOSKIPTEXT] = static_cast<int>(game.Settings.SkipSpeech);
    options[OPT_PORTRAITSIDE] = static_cast<int>(game.Settings.SpeechPortraitSide);
    options[OPT_SPEECHTYPE] = static_cast<int>(game.Settings.SpeechStyle);
    options[OPT_SPLITRESOURCES] = ParseSplitResources(game.Settings.SplitResources);
    options[OPT_BASESCRIPTAPI] = ParseScriptApiVersion(game.Settings.ScriptAPIVersion);
    options[OPT_SCRIPTCOMPATLEV] = ParseScriptApiVersion(game.Settings.ScriptCompatLevel);
    options[OPT_GAMETEXTENCODING] = ParseGameTextEncodingCodePage(game.Settings.GameTextEncoding);
    options[OPT_DEBUGMODE] = game.Settings.DebugMode ? 1 : 0;
    options[OPT_COMPRESSSPRITES] = static_cast<int>(game.Settings.CompressSpritesType);
    options[OPT_LETTERBOX] = game.Settings.LetterboxMode ? 1 : 0;
    options[OPT_NATIVECOORDINATES] = game.Settings.UseLowResCoordinatesInScript ? 0 : 1;
    options[OPT_SAFEFILEPATHS] = 1;
    options[OPT_RELATIVEASSETRES] = game.Settings.AllowRelativeAssetResolutions ? 1 : 0;
    options[OPT_STRICTSTRINGS] = game.Settings.EnforceNewStrings ? 1 : 0;
    options[OPT_STRICTSCRIPTING] = game.Settings.EnforceObjectBasedScript ? 1 : 0;
    options[OPT_HANDLEINVCLICKS] = game.Settings.HandleInvClicksInScript ? 1 : 0;
    options[OPT_FIXEDINVCURSOR] = game.Settings.InventoryCursors ? 0 : 1;
    options[OPT_GLOBALTALKANIMSPD] = game.Settings.UseGlobalSpeechAnimationDelay
        ? game.Settings.GlobalSpeechAnimationDelay
        : (-game.Settings.GlobalSpeechAnimationDelay - 1);
    options[OPT_LEFTTORIGHTEVAL] = game.Settings.LeftToRightPrecedence ? 1 : 0;
    options[OPT_MOUSEWHEEL] = game.Settings.MouseWheelEnabled ? 1 : 0;
    options[OPT_PIXPERFECT] = game.Settings.PixelPerfect ? 1 : 0;
    options[OPT_RUNGAMEDLGOPTS] = game.Settings.RunGameLoopsWhileDialogOptionsDisplayed ? 1 : 0;
    options[OPT_SAVESCREENSHOT] = game.Settings.SaveScreenshots ? 1 : 0;
    options[OPT_CHARTURNWHENFACE] = game.Settings.TurnBeforeFacing ? 1 : 0;
    options[OPT_CHARTURNWHENWALK] = game.Settings.TurnBeforeWalking ? 1 : 0;
    options[OPT_WALKONLOOK] = game.Settings.WalkInLookMode ? 1 : 0;
    options[OPT_CLIPGUICONTROLS] = game.Settings.ClipGUIControls ? 1 : 0;
    options[OPT_SCALECHAROFFSETS] = game.Settings.ScaleCharacterSpriteOffsets ? 1 : 0;
    options[OPT_WALKSPEEDABSOLUTE] = game.Settings.ScaleMovementSpeedWithMaskResolution ? 0 : 1;
    options[OPT_SAVESCREENSHOTLAYER] = -1;
    options[OPT_DIALOGOPTIONSAPI] = game.Settings.UseOldCustomDialogOptionsAPI ? -1 : 1;
    options[OPT_KEYHANDLEAPI] = game.Settings.UseOldKeyboardHandling ? 0 : 1;
    options[OPT_TWCUSTOM] = game.Settings.TextWindowGUI;
    options[OPT_THOUGHTGUI] = game.Settings.ThoughtGUI;
    options[OPT_DISABLEOFF] = static_cast<int>(game.Settings.WhenInterfaceDisabled);
    options[OPT_RENDERATSCREENRES] = static_cast<int>(game.Settings.RenderAtScreenResolution);
    options[OPT_FADETYPE] = static_cast<int>(game.Settings.RoomTransition);
    options[OPT_LIPSYNCTEXT] = game.LipSync == DataUtil::kLipSync_Text ? 1 : 0;
    options[OPT_VOICECLIPNAMERULE] = game.Settings.UseOldVoiceClipNaming ? 0 : 1;
    options[OPT_GAMEFPS] = game.Settings.GameFPS;
    options[OPT_GUICONTROLMOUSEBUT] = game.Settings.GUIHandleOnlyLeftMouseButton ? 1 : 0;
    options[OPT_DISPLAYSINGLEDIALOGOPTION] = game.Settings.DisplaySingleDialogOption ? 1 : 0;
    options[OPT_TURNORDERPRIORITY] = game.Settings.TurnOrderPriority;
    options[OPT_TEXTBOXCLAIMSKEYS] = game.Settings.TextBoxKeyClaimStyle;
    return options;
}

// Read by GameSetupStructBase::ReadFromFile() in
// Common/ac/gamesetupstructbase.cpp, at the start of ReadGameData().
void WriteGameSetupStructBase(const DataUtil::GameData &game, Stream *out, soff_t &ext_offset_pos)
{
    String game_name = game.Settings.GameName;
    if (game_name.IsEmpty())
        game_name = game.Settings.GameFileName;
    StrUtil::WriteFixedString(game_name, GameSetupStructBase::LEGACY_GAME_NAME_LENGTH, out);
    out->WriteInt16(0); // alignment padding to int32

    const std::vector<int32_t> options = BuildGameOptions(game);

    for (int i = 0; i < GameSetupStructBase::MAX_OPTIONS; ++i)
        out->WriteInt32(options[i]);

    for (int i = 0; i < 256; ++i)
        out->WriteByte(i < static_cast<int>(game.Palette.size()) &&
            game.Palette[i].ColourType == DataUtil::kPaletteColourType_Background ? 2 : 0);
    for (int i = 0; i < 256; ++i)
    {
        const DataUtil::PaletteEntryData entry = i < static_cast<int>(game.Palette.size()) ?
            game.Palette[i] : DataUtil::PaletteEntryData{};
        out->WriteByte(entry.Red / 4);
        out->WriteByte(entry.Green / 4);
        out->WriteByte(entry.Blue / 4);
        out->WriteByte(255); // opaque
    }

    const int num_views = static_cast<int>(game.Views.size());
    const int num_characters = static_cast<int>(game.Characters.size());
    const int num_inventory = static_cast<int>(game.Inventory.size());
    const int num_dialogs = 0; // old dialog block is not used by this tool
    const int num_fonts = static_cast<int>(game.Fonts.size());
    const int num_gui = static_cast<int>(game.GUI.size());
    const int num_cursors = static_cast<int>(game.Cursors.size());

    out->WriteInt32(num_views); // ViewCount
    out->WriteInt32(num_characters); // Characters.Count
    out->WriteInt32(game.PlayerCharacter); // PlayerCharacter.ID
    out->WriteInt32(game.Settings.MaximumScore);
    out->WriteInt16(static_cast<int16_t>(num_inventory + 1)); // add dummy item at index 0
    out->WriteInt16(0); // alignment padding
    out->WriteInt32(0); // was game.Dialogs.Count, write 0 for old format entries, we use "v363_dialogsnew" extension
    out->WriteInt32(0); // numdlgmessage, deprecated
    out->WriteInt32(num_fonts); // Fonts.Count
    out->WriteInt32(static_cast<int32_t>(game.Settings.ColorDepth)); // color_depth in bytes per pixel
    out->WriteInt32(0); // target_win
    out->WriteInt32(game.Settings.DialogOptionsBullet);
    out->WriteInt16(static_cast<int16_t>(game.Settings.InventoryHotspotMarkerStyle != DataUtil::kInventoryHotspot_None ?
        game.Settings.InventoryHotspotMarkerDotColor : 0));
    out->WriteInt16(static_cast<int16_t>(game.Settings.InventoryHotspotMarkerCrosshairColor));
    out->WriteInt32(game.Settings.UniqueID);
    out->WriteInt32(num_gui); // GUIs.Count
    out->WriteInt32(num_cursors); // Cursors.Count

    // IGNORE LETTERBOX RESOLUTION
    int game_width = 320;
    int game_height = 200;
    const bool has_resolution = ParseResolution(game.Settings.CustomResolution, game_width, game_height);
    if (has_resolution && game_width > 0 && game_height > 0)
    {
        out->WriteInt32(kGameResolution_Custom);
        out->WriteInt32(game_width);
        out->WriteInt32(game_height);
    }
    else
    {
        out->WriteInt32(kGameResolution_Custom);
        out->WriteInt32(320);
        out->WriteInt32(200);
    }

    out->WriteInt32(game.LipSyncDefaultFrame);
    out->WriteInt32(game.Settings.InventoryHotspotMarkerStyle == DataUtil::kInventoryHotspot_Sprite ?
        game.Settings.InventoryHotspotMarkerSprite : 0);
    // reserved; 16 ints
    for (int i = 0; i < GameSetupStructBase::NUM_INTS_RESERVED; ++i)
        out->WriteInt32(0);

    ext_offset_pos = out->GetPosition();
    out->WriteInt32(0); // extension offset - none

    // MAXGLOBALMES; write 500 ints
    for (int i = 0; i < MAXGLOBALMES; ++i)
        out->WriteInt32(i < static_cast<int>(game.GlobalMessages.size()) && !game.GlobalMessages[i].IsEmpty() ? 1 : 0);

    out->WriteInt32(1); // dict != null
    out->WriteInt32(0); // globalscript != null
    out->WriteInt32(0); // chars != null
    out->WriteInt32(0); // compiled scripts are loaded externally since 3.6.1
}

// Read by GameSetupStruct::read_savegame_info() in
// Common/ac/gamesetupstruct.cpp.
void WriteSaveGameInfo(const DataUtil::GameData &game, Stream *out)
{
    String guid = game.Settings.GUIDAsString;
    StrUtil::WriteFixedString(guid, MAX_GUID_LENGTH, out);
    StrUtil::WriteFixedString(game.Settings.SaveGameFileExtension, MAX_SG_EXT_LENGTH, out);
    StrUtil::WriteFixedString(game.Settings.SaveGameFolderName, LEGACY_MAX_SG_FOLDER_LEN, out);
}

// Read by GameSetupStruct::read_font_infos() in
// Common/ac/gamesetupstruct.cpp.
void WriteFontBlock(const DataUtil::GameData &game, Stream *out)
{
    for (const auto &font : game.Fonts)
        WriteFontInfo(out, font);
}

// Read by ReadSpriteFlags() in Common/game/main_game_file.cpp.
void WriteSpriteFlags(const DataUtil::GameData &game, Stream *out)
{
    int topmost = -1;
    for (const auto &sprite : game.Sprites)
        topmost = std::max(topmost, sprite.Slot);
    out->WriteInt32(topmost + 1);
    std::vector<uint8_t> flags(topmost + 1, 0);
    for (const auto &sprite : game.Sprites)
    {
        if (sprite.Slot < 0 || sprite.Slot > topmost) continue;
        if (sprite.Resolution != DataUtil::kSpriteImport_Real) flags[sprite.Slot] |= SPF_VAR_RESOLUTION;
        if (sprite.Resolution == DataUtil::kSpriteImport_HighRes) flags[sprite.Slot] |= SPF_HIRES;
        if (sprite.AlphaChannel) flags[sprite.Slot] |= SPF_ALPHACHANNEL;
    }
    if (!flags.empty())
        out->Write(flags.data(), flags.size());
}

// Read by GameSetupStruct::ReadInvInfo() in Common/ac/gamesetupstruct.cpp,
// which calls InventoryItemInfo::ReadFromFile().
void WriteInventoryBlock(const DataUtil::GameData &game, Stream *out)
{
    // Add dummy item at index 0
    DataUtil::InventoryItemData dummy_item;
    WriteInventoryItem(out, dummy_item);
    for (const auto &item_ref : game.Inventory)
        WriteInventoryItem(out, item_ref);
}

// Read by GameSetupStruct::read_cursors()/ReadMouseCursors() in
// Common/ac/gamesetupstruct.cpp.
void WriteCursorBlock(const DataUtil::GameData &game, Stream *out)
{
    for (const auto &cursor_ref : game.Cursors)
        WriteCursor(out, cursor_ref);
}

// Read by GameSetupStruct::read_interaction_scripts() in
// Common/ac/gamesetupstruct.cpp. Its inventory loop intentionally starts at
// slot 1, unlike the v361_objnames extension, which includes unused slot 0.
void WriteInteractionScriptsBlock(const DataUtil::GameData &game, Stream *out)
{
    for (size_t i = 0; i < game.Characters.size(); ++i)
        WriteEmptyInteractionEvents(out);
    for (size_t i = 0; i < game.Inventory.size(); ++i)
        WriteEmptyInteractionEvents(out);
}

// Read by GameSetupStruct::read_words_dictionary(), then
// WordsDictionary::ReadFromFile() in Common/ac/wordsdictionary.cpp.
void WriteTextParserDictionary(const DataUtil::GameData &game, Stream *out)
{
    std::vector<DataUtil::TextParserWordData> words;
    for (const auto &item : game.ParserWords)
    {
        String remaining = item.Word;
        while (!remaining.IsEmpty())
        {
            const int comma = remaining.FindChar(',');
            String word = comma >= 0 ? remaining.Left(comma) : remaining;
            word.Trim();
            if (!word.IsEmpty())
            {
                DataUtil::TextParserWordData parsed;
                parsed.Word = word;
                parsed.WordGroup = item.WordGroup;
                words.push_back(parsed);
            }
            if (comma < 0) break;
            remaining = remaining.Mid(comma + 1);
        }
    }
    out->WriteInt32(static_cast<int32_t>(words.size()));
    for (const auto &word : words)
    {
        write_string_encrypt(out, word.Word.GetCStr());
        out->WriteInt16(static_cast<int16_t>(word.WordGroup));
    }
}

// Read by ReadViews() in Common/game/main_game_file.cpp, which calls
// ViewStruct::ReadFromFile().
void WriteViewsBlock(const DataUtil::GameData &game, Stream *out)
{
    for (size_t i = 0; i < game.Views.size(); ++i)
        WriteView(out, game, game.Views[i]);
}

// Read by GameSetupStruct::read_characters()/ReadCharacters() in
// Common/ac/gamesetupstruct.cpp.
void WriteCharactersBlock(const DataUtil::GameData &game, Stream *out)
{
    for (size_t i = 0; i < game.Characters.size(); ++i)
        WriteCharacter(out, game, game.Characters[i], static_cast<int>(i));
}

// Read by GameSetupStruct::read_lipsync() in Common/ac/gamesetupstruct.cpp.
void WriteLipSyncBlock(const DataUtil::GameData &game, Stream *out)
{
    for (int i = 0; i < MAXLIPSYNCFRAMES; ++i)
        StrUtil::WriteFixedString(i < static_cast<int>(game.LipSyncFrames.size()) ? game.LipSyncFrames[i] : String(), 50, out);
}

// Read by GameSetupStruct::read_messages() in Common/ac/gamesetupstruct.cpp.
void WriteGlobalMessagesBlock(const DataUtil::GameData &game, Stream *out)
{
    for (const auto &message : game.GlobalMessages)
        if (!message.IsEmpty()) write_string_encrypt(out, message.GetCStr());
}

// Read by GUI::ReadGUI() in Common/gui/guimain.cpp; that dispatches to
// GUIMain and each concrete GUI control's ReadFromFile().
void WriteGuiBlock(const DataUtil::GameData &game, Stream *out)
{
    std::vector<std::shared_ptr<DataUtil::GUIButtonData>> buttons;
    std::vector<std::shared_ptr<DataUtil::GUILabelData>> labels;
    std::vector<std::shared_ptr<DataUtil::GUIInventoryData>> inv_windows;
    std::vector<std::shared_ptr<DataUtil::GUISliderData>> sliders;
    std::vector<std::shared_ptr<DataUtil::GUITextBoxData>> text_boxes;
    std::vector<std::shared_ptr<DataUtil::GUIListBoxData>> list_boxes;
    std::vector<std::vector<uint32_t>> gui_control_refs(game.GUI.size());

    for (size_t gui_index = 0; gui_index < game.GUI.size(); ++gui_index)
    {
        for (const auto &control : game.GUI[gui_index].Controls)
        {
            uint32_t packed_ref = 0;
            if (auto button = std::dynamic_pointer_cast<DataUtil::GUIButtonData>(control))
            {
                packed_ref = (kGUIButton << 16) | static_cast<uint32_t>(buttons.size());
                buttons.push_back(button);
            }
            else if (auto label = std::dynamic_pointer_cast<DataUtil::GUILabelData>(control))
            {
                packed_ref = (kGUILabel << 16) | static_cast<uint32_t>(labels.size());
                labels.push_back(label);
            }
            else if (auto inv = std::dynamic_pointer_cast<DataUtil::GUIInventoryData>(control))
            {
                packed_ref = (kGUIInvWindow << 16) | static_cast<uint32_t>(inv_windows.size());
                inv_windows.push_back(inv);
            }
            else if (auto slider = std::dynamic_pointer_cast<DataUtil::GUISliderData>(control))
            {
                packed_ref = (kGUISlider << 16) | static_cast<uint32_t>(sliders.size());
                sliders.push_back(slider);
            }
            else if (auto text_box = std::dynamic_pointer_cast<DataUtil::GUITextBoxData>(control))
            {
                packed_ref = (kGUITextBox << 16) | static_cast<uint32_t>(text_boxes.size());
                text_boxes.push_back(text_box);
            }
            else if (auto list_box = std::dynamic_pointer_cast<DataUtil::GUIListBoxData>(control))
            {
                packed_ref = (kGUIListBox << 16) | static_cast<uint32_t>(list_boxes.size());
                list_boxes.push_back(list_box);
            }
            else
            {
                continue;
            }
            gui_control_refs[gui_index].push_back(packed_ref);
        }
    }

    out->WriteInt32(GUIMAGIC);
    out->WriteInt32(kGuiVersion_Current);
    out->WriteInt32(static_cast<int32_t>(game.GUI.size()));
    for (size_t i = 0; i < game.GUI.size(); ++i)
        WriteGui(out, game.GUI[i], gui_control_refs[i]);

    out->WriteInt32(static_cast<int32_t>(buttons.size()));
    for (const auto &button : buttons)
        WriteGuiButton(out, *button);
    out->WriteInt32(static_cast<int32_t>(labels.size()));
    for (const auto &label : labels)
        WriteGuiLabel(out, *label);
    out->WriteInt32(static_cast<int32_t>(inv_windows.size()));
    for (const auto &inv : inv_windows)
        WriteGuiInvWindow(out, *inv);
    out->WriteInt32(static_cast<int32_t>(sliders.size()));
    for (const auto &slider : sliders)
        WriteGuiSlider(out, *slider);
    out->WriteInt32(static_cast<int32_t>(text_boxes.size()));
    for (const auto &text_box : text_boxes)
        WriteGuiTextBox(out, *text_box);
    out->WriteInt32(static_cast<int32_t>(list_boxes.size()));
    for (const auto &list_box : list_boxes)
        WriteGuiListBox(out, *list_box);
}

// Read by ReadPlugins() in Common/game/main_game_file.cpp.
void WritePluginsBlock(const DataUtil::GameData &game, Stream *out)
{
    out->WriteInt32(1); // plugin data format version
    out->WriteInt32(static_cast<int32_t>(game.Plugins.size()));
    for (const auto &plugin : game.Plugins)
    {
        StrUtil::WriteCStr(plugin.Name, out);
        out->WriteInt32(static_cast<int32_t>(plugin.Data.size()));
        if (!plugin.Data.empty()) out->Write(plugin.Data.data(), plugin.Data.size());
    }
}

// Read by the schema/property-values portion of
// GameSetupStruct::read_customprops() in Common/ac/gamesetupstruct.cpp.
void WriteCustomPropertiesBlock(const DataUtil::GameData &game, Stream *out)
{
    WritePropertySchemaBlock(out, game.PropertySchema);
    for (const auto &character : game.Characters)
        WritePropertyValues(out, character.Properties);
    // Add dummy item at index 0
    WriteEmptyPropertyValues(out);
    for (const auto &item : game.Inventory)
        WritePropertyValues(out, item.Properties);
}

// Read by the trailing viewNames/invScriptNames/dialogScriptNames portion of
// GameSetupStruct::read_customprops() in Common/ac/gamesetupstruct.cpp.
void WriteLegacyScriptNamesBlock(const DataUtil::GameData &game, Stream *out)
{
    for (size_t i = 0; i < game.Views.size(); ++i)
    {
        const String &name = game.Views[i].ScriptName.IsEmpty() ? game.Views[i].TypeName : game.Views[i].ScriptName;
        StrUtil::WriteCStr(name, out);
    }

    out->WriteInt8(0); // inventory slot 0 is unused, so its name is a single NUL byte
    for (size_t i = 0; i < game.Inventory.size(); ++i)
    {
        const String &name = game.Inventory[i].ScriptName.IsEmpty() ? game.Inventory[i].TypeName : game.Inventory[i].ScriptName;
        StrUtil::WriteCStr(name, out);
    }
}

// Read by GameSetupStruct::read_audio() in Common/ac/gamesetupstruct.cpp.
void WriteAudioBlock(const DataUtil::GameData &game, Stream *out)
{
    // Add a fixed voice-over audio type at index 0
    const int audio_type_count = static_cast<int>(game.AudioTypes.size()) + 1;
    out->WriteInt32(audio_type_count);
    DataUtil::AudioTypeData voice_type;
    voice_type.MaxChannels = 1;
    WriteAudioType(out, voice_type, 0);
    for (size_t i = 0; i < game.AudioTypes.size(); ++i)
        WriteAudioType(out, game.AudioTypes[i], static_cast<int>(i) + 1);

    out->WriteInt32(static_cast<int32_t>(game.AudioClips.size()));
    for (size_t i = 0; i < game.AudioClips.size(); ++i)
        WriteAudioClip(out, game.AudioClips[i], static_cast<int>(i));

    out->WriteInt32(GetAudioID(game, game.Settings.PlaySoundOnScore));
}

// Read by GameSetupStruct::read_room_names() in
// Common/ac/gamesetupstruct.cpp.
void WriteRoomNamesBlock(const DataUtil::GameData &game, Stream *out)
{
    if (!game.Settings.DebugMode)
        return;

    out->WriteInt32(static_cast<int32_t>(game.Rooms.size()));
    for (const auto &room : game.Rooms)
        WriteRoomName(out, room.first, room.second);
}

// Read by InteractionEvents::CreateFromStream_v362() in
// Common/game/interactions.cpp, called by ReadInteractionScriptModules().
void WriteInteractionEvents(Stream *out, const String &module,
    const std::vector<String> &events)
{
    out->WriteInt32(kInterEvents_v362);
    StrUtil::WriteString(module, out);
    out->WriteInt32(static_cast<int32_t>(events.size()));
    for (const auto &event : events)
        StrUtil::WriteString(event, out);
}

// Read by InteractionEvents::CreateFromStream_v362() in
// Common/game/interactions.cpp for unused inventory slot 0.
void WriteEmptyInteractionEventsV362(Stream *out)
{
    out->WriteInt32(kInterEvents_v362);
    StrUtil::WriteString("", out);
    out->WriteInt32(0);
}

// Read by GUIObject::ReadFromFile_Ext363() in Common/gui/guiobject.cpp as the
// common prefix of each concrete GUI control's extension reader.
void WriteGuiControlLooks363(Stream *out, const DataUtil::GUIControlData &control)
{
    out->WriteInt32(control.BackgroundColor);
    out->WriteInt32(control.BorderColor);
    out->WriteInt32(control.BorderWidth);
    out->WriteInt32(control.PaddingX);
    out->WriteInt32(control.PaddingY);
    // reserved
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
    out->WriteInt32(0);
}

// Read by GameDataExtReader::ReadBlock()'s "v360_fonts" branch in
// Common/game/main_game_file.cpp.
void WriteExt360Fonts(Stream *out, const DataUtil::GameData &game)
{
    for (const auto &font : game.Fonts)
    {
        out->WriteInt32(font.AutoOutlineThickness);
        out->WriteInt32(font.AutoOutlineStyle);
        out->WriteInt32(font.CharacterSpacing);
        out->WriteInt32(font.CustomHeightValue);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
}

// Read by GameDataExtReader::ReadBlock()'s "v360_cursors" branch in
// Common/game/main_game_file.cpp.
void WriteExt360Cursors(Stream *out, const DataUtil::GameData &game)
{
    for (const auto &cursor : game.Cursors)
    {
        out->WriteInt32(cursor.AnimationDelay);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
}

// Read by GameDataExtReader::ReadBlock()'s "v361_objnames" branch in
// Common/game/main_game_file.cpp. This inventory list includes unused slot 0;
// the legacy read_interaction_scripts() inventory list intentionally does not.
void WriteExt361ObjNames(Stream *out, const DataUtil::GameData &game)
{
    StrUtil::WriteString(game.Settings.GameName, out);
    StrUtil::WriteString(game.Settings.SaveGameFolderName, out);
    out->WriteInt32(static_cast<int32_t>(game.Characters.size()));
    for (const auto &character : game.Characters)
    {
        StrUtil::WriteString(character.ScriptName, out);
        StrUtil::WriteString(character.RealName, out);
    }
    // Add dummy item at index 0
    out->WriteInt32(static_cast<int32_t>(game.Inventory.size() + 1));
    StrUtil::WriteString("", out);
    for (const auto &item : game.Inventory)
        StrUtil::WriteString(item.Description, out);
    out->WriteInt32(static_cast<int32_t>(game.Cursors.size()));
    for (const auto &cursor : game.Cursors)
        StrUtil::WriteString(cursor.ScriptName, out);
    out->WriteInt32(static_cast<int32_t>(game.AudioClips.size()));
    for (const auto &clip : game.AudioClips)
    {
        StrUtil::WriteString(clip.ScriptName, out);
        StrUtil::WriteString(clip.CacheFileName, out);
    }
}

// Read by GameDataExtReader::ReadBlock()'s "v362_interevent2" branch, then
// ReadInteractionScriptModules(), in Common/game/main_game_file.cpp.
void WriteExt362Interactions(Stream *out, const DataUtil::GameData &game)
{
    // Embedded scripts are intentionally absent; script objects are supplied externally.
    StrUtil::WriteString("", out);
    StrUtil::WriteString("", out);
    out->WriteInt32(0); // script module filename count
    out->WriteInt32(static_cast<int32_t>(game.Characters.size()));
    for (const auto &character : game.Characters)
        WriteInteractionEvents(out, character.ScriptModule, character.InteractionEvents);
    out->WriteInt32(static_cast<int32_t>(game.Inventory.size() + 1));
    WriteEmptyInteractionEventsV362(out);
    for (const auto &item : game.Inventory)
        WriteInteractionEvents(out, item.ScriptModule, item.InteractionEvents);
    out->WriteInt32(static_cast<int32_t>(game.GUI.size()));
    for (const auto &gui : game.GUI)
        StrUtil::WriteString(gui.ScriptModule, out);
}

// Read by GameDataExtReader::ReadBlock()'s "v363_gameinfo" branch, using
// StrUtil::ReadStringMap(), in Common/game/main_game_file.cpp.
void WriteExt363GameInfo(Stream *out, const DataUtil::GameData &game)
{
    String release_date = game.Settings.ReleaseDate;
    int year = 0, month = 0, day = 0;
    if (std::sscanf(release_date.GetCStr(), "%d-%d-%d", &year, &month, &day) == 3)
        release_date = String::FromFormat("%02d.%02d.%04d", day, month, year);
    String text_lang = game.Settings.GameTextLanguage;
    text_lang.Replace('-', '_');
    const std::pair<const char*, String> info[] = {
        {"title", game.Settings.GameName}, {"description", game.Settings.Description},
        {"dev_name", game.Settings.DeveloperName}, {"dev_url", game.Settings.DeveloperURL},
        {"genre", game.Settings.Genre}, {"release_date", release_date},
        {"version", game.Settings.Version}, {"text_lang", text_lang}
    };
    out->WriteInt32(8);
    for (const auto &item : info)
    {
        StrUtil::WriteString(item.first, out);
        StrUtil::WriteString(item.second, out);
    }
}

template <typename T> std::vector<std::shared_ptr<T>> CollectGuiControls(const DataUtil::GameData &game)
{
    std::vector<std::shared_ptr<T>> result;
    for (const auto &gui : game.GUI)
        for (const auto &control : gui.Controls)
            if (auto typed = std::dynamic_pointer_cast<T>(control)) result.push_back(typed);
    return result;
}

// Read by GameDataExtReader::ReadBlock()'s "v363_guictrls2" branch in
// Common/game/main_game_file.cpp, which calls each concrete GUI control's
// ReadFromFile_Ext363().
void WriteExt363GuiControls(Stream *out, const DataUtil::GameData &game)
{
    const auto buttons = CollectGuiControls<DataUtil::GUIButtonData>(game);
    out->WriteInt32(static_cast<int32_t>(buttons.size()));
    for (const auto &button : buttons)
    {
        WriteGuiControlLooks363(out, *button);
        const bool dynamic = button->ColorStyle == DataUtil::kButtonColor_Dynamic ||
            button->ColorStyle == DataUtil::kButtonColor_DynamicFlat;
        out->WriteInt32((dynamic ? AGS::Common::kButton_DynamicColors : 0) |
            (button->ColorStyle == DataUtil::kButtonColor_DynamicFlat ? AGS::Common::kButton_FlatStyle : 0));
        out->WriteInt32(button->BorderShadeColor);
        out->WriteInt32(button->MouseOverBackgroundColor);
        out->WriteInt32(button->PushedBackgroundColor);
        out->WriteInt32(button->MouseOverBorderColor);
        out->WriteInt32(button->PushedBorderColor);
        out->WriteInt32(button->MouseOverTextColor);
        out->WriteInt32(button->PushedTextColor);
        out->WriteInt32(button->TextOutlineColor);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
    const auto labels = CollectGuiControls<DataUtil::GUILabelData>(game);
    out->WriteInt32(static_cast<int32_t>(labels.size()));
    for (const auto &label : labels)
    {
        WriteGuiControlLooks363(out, *label);
        out->WriteInt32(label->TextOutlineColor);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
    const auto invs = CollectGuiControls<DataUtil::GUIInventoryData>(game);
    out->WriteInt32(static_cast<int32_t>(invs.size()));
    for (const auto &inv : invs)
    {
        WriteGuiControlLooks363(out, *inv);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0); 
    }
    const auto sliders = CollectGuiControls<DataUtil::GUISliderData>(game);
    out->WriteInt32(static_cast<int32_t>(sliders.size()));
    for (const auto &slider : sliders)
    {
        WriteGuiControlLooks363(out, *slider);
        out->WriteInt32(slider->HandleColor);
        out->WriteInt32(slider->BorderShadeColor);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
    const auto textboxes = CollectGuiControls<DataUtil::GUITextBoxData>(game);
    out->WriteInt32(static_cast<int32_t>(textboxes.size()));
    for (const auto &textbox : textboxes)
    {
        WriteGuiControlLooks363(out, *textbox);
        out->WriteInt32(textbox->TextAlignment);
        out->WriteInt32(textbox->TextOutlineColor);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
    const auto listboxes = CollectGuiControls<DataUtil::GUIListBoxData>(game);
    out->WriteInt32(static_cast<int32_t>(listboxes.size()));
    for (const auto &listbox : listboxes)
    {
        WriteGuiControlLooks363(out, *listbox);
        out->WriteInt32(listbox->TextOutlineColor);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);
    }
}

// Read as one extension record by DataExtReader::Read() in
// Common/data/data_ext.cpp; GameDataExtReader::ReadBlock() reads its payload.
void WriteExtension(Stream *out, const char *id, const DataUtil::GameData &game,
    ExtensionWriter writer)
{
    out->WriteInt8(0);
    StrUtil::WriteFixedString(id, 16, out);
    const soff_t length_pos = out->GetPosition();
    out->WriteInt64(0);
    const soff_t data_pos = out->GetPosition();
    writer(out, game);
    const soff_t end_pos = out->GetPosition();
    out->Seek(length_pos, kSeekBegin);
    out->WriteInt64(end_pos - data_pos);
    out->Seek(end_pos, kSeekBegin);
}

// Read by GameDataExtReader::ReadBlock()'s "v363_dialogsnew" branch, then
// DialogTopic::ReadFromFile_v363() in Common/ac/dialogtopic.cpp.
void WriteExt363Dialogs(Stream *out, const DataUtil::GameData &game)
{
    out->WriteInt32(static_cast<int32_t>(game.Dialogs.size()));
    for (const auto &dialog : game.Dialogs)
    {
        StrUtil::WriteString(dialog.ScriptName, out);
        out->WriteInt32(dialog.ShowTextParser ? DTFLG_SHOWPARSER : 0);
        // reserved
        out->WriteInt32(0);
        out->WriteInt32(0);
        out->WriteInt32(0);

        out->WriteInt32(static_cast<int32_t>(dialog.Options.size()));
        for (const auto &option : dialog.Options)
        {
            StrUtil::WriteString(option.Text, out);
            int flags = 0;
            if (!option.Say) flags |= DFLG_NOREPEAT;
            if (option.Show) flags |= DFLG_ON;
            out->WriteInt32(flags);
            // reserved
            out->WriteInt32(0);
            out->WriteInt32(0);
            out->WriteInt32(0);
        }
    }
}

// Read by DataExtReader::Read() through GameDataExtReader in
// Common/game/main_game_file.cpp. The patched offset is read earlier by
// GameSetupStructBase::ReadFromFile().
void WriteExtensions(Stream *out, const DataUtil::GameData &game, soff_t ext_offset_pos)
{
    const soff_t ext_offset = out->GetPosition();
    out->Seek(ext_offset_pos, kSeekBegin);
    out->WriteInt32(static_cast<int32_t>(ext_offset));
    out->Seek(ext_offset, kSeekBegin);
    WriteExtension(out, "v360_fonts", game, WriteExt360Fonts);
    WriteExtension(out, "v360_cursors", game, WriteExt360Cursors);
    WriteExtension(out, "v361_objnames", game, WriteExt361ObjNames);
    WriteExtension(out, "v362_interevent2", game, WriteExt362Interactions);
    WriteExtension(out, "v363_gameinfo", game, WriteExt363GameInfo);
    WriteExtension(out, "v363_dialogsnew", game, WriteExt363Dialogs);
    WriteExtension(out, "v363_guictrls2", game, WriteExt363GuiControls);
    out->WriteInt8(static_cast<uint8_t>(0xff));
}

// Read by OpenMainGameFileBase() in Common/game/main_game_file.cpp before
// ReadGameData() is entered.
void WriteHeaderBlock(const DataUtil::GameData &game, Stream *out)
{
    StrUtil::WriteFixedString("Adventure Creator Game File v2", 30, out);
    out->WriteInt32(kGameVersion_Current);

    // Preserve the editor version recorded in Game.agf.
    const String compiled_with = game.EditorVersion.IsEmpty() ? String("3.6.3.12") : game.EditorVersion;
    out->WriteInt32(static_cast<int32_t>(compiled_with.GetLength()));
    StrUtil::WriteFixedString(compiled_with, compiled_with.GetLength(), out);

    out->WriteInt32(0); // no extended capabilities
}

} // namespace DataFileWriter

namespace DataUtil
{

// Read back by ReadGameData() in Common/game/main_game_file.cpp after
// OpenMainGameFileBase() has consumed the file header.
HError WriteGameData28(const GameData &game, std::unique_ptr<Stream> &&out)
{
    using namespace AGS::DataFileWriter;

    if (!out)
        return new Error("WriteGameData28: Invalid output stream.");

    Stream *stream = out.get();

    WriteHeaderBlock(game, stream);
    soff_t ext_offset_pos = 0;
    WriteGameSetupStructBase(game, stream, ext_offset_pos);
    WriteSaveGameInfo(game, stream);
    WriteFontBlock(game, stream);
    WriteSpriteFlags(game, stream);
    WriteInventoryBlock(game, stream);
    WriteCursorBlock(game, stream);
    WriteInteractionScriptsBlock(game, stream);
    WriteTextParserDictionary(game, stream);
    // Compiled global/dialog scripts and module list are omitted intentionally;
    // HasCCScript is false in GameSetupStructBase, so no zero payload is needed here.
    WriteViewsBlock(game, stream);
    WriteCharactersBlock(game, stream);
    WriteLipSyncBlock(game, stream);
    WriteGlobalMessagesBlock(game, stream);
    WriteGuiBlock(game, stream);
    WritePluginsBlock(game, stream);
    WriteCustomPropertiesBlock(game, stream);
    WriteLegacyScriptNamesBlock(game, stream);
    WriteAudioBlock(game, stream);
    WriteRoomNamesBlock(game, stream);
    WriteExtensions(stream, game, ext_offset_pos);

    if (!out->Flush())
        return new Error("WriteGameData28: Failed to flush game data output stream.");
    return HError::None();
}

} // namespace DataUtil
} // namespace AGS
