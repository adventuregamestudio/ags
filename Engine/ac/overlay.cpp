//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/overlay.h"
#include <algorithm>
#include <queue>
#include "ac/common.h"
#include "ac/view.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_overlay.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern int displayed_room;
extern int face_talking;
extern std::vector<ViewStruct> views;
extern IGraphicsDriver *gfxDriver;

// TODO: consider some kind of a "object pool" template,
// which handles this kind of storage; share with ManagedPool's handles?
std::vector<ScreenOverlay> screenover;
std::queue<int32_t> over_free_ids;

// Gets an actual ScreenOverlay object from its ScriptOverlay reference,
// validate object, throw an error on failure
static ScreenOverlay *GetOverlayValidate(const char *apiname, ScriptOverlay *scover)
{
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quitprintf("!%s: invalid overlay specified", apiname);
    return over;
}

void Overlay_Remove(ScriptOverlay *sco)
{
    sco->Remove();
}

void Overlay_SetText(ScriptOverlay *scover, int width, int fontid, int text_color, const char *text)
{
    auto *over = GetOverlayValidate("Overlay.SetText", scover);

    Overlay_SetText(*over, over->GetX(), over->GetY(), width, fontid, text_color, text);
}

void Overlay_SetText(ScreenOverlay &over, int x, int y, int width, int fontid, int text_color, const char *text)
{
    // TODO: find a nice way to refactor and share these code pieces
    // with Overlay_CreateTextCore
    int text_pos = kDisplayTextPos_Normal;
    if (x == OVR_AUTOPLACE)
    {
        text_pos = kDisplayTextPos_Overchar;
    }
    else
    {
        // NOTE: this was not documented, but apparently passing x < 0 or y < 0
        // to SetTextOverlay actually made it centered on screen
        text_pos = get_textpos_from_scriptcoords(x, y, false);
    }
    // allow DisplaySpeechBackground to be shrunk
    DisplayTextShrink allow_shrink = (x == OVR_AUTOPLACE) ? kDisplayTextShrink_Left : kDisplayTextShrink_None;

    // from Overlay_CreateTextCore
    width = data_to_game_coord(width);
    if (width < 8) width = play.GetUIViewport().GetWidth() / 2;
    if (text_color == 0) text_color = 16;

    const char *draw_text = get_translation(text);
    // Skip a voice-over token, if present
    draw_text = skip_voiceover_token(draw_text);

    // Recreate overlay image
    int dummy_x = x, dummy_y = y, adj_x = x, adj_y = y;
    bool has_alpha = false;
    std::unique_ptr<Bitmap> image = create_textual_image(draw_text,
        DisplayTextLooks(kDisplayTextStyle_TextWindow, (DisplayTextPosition)text_pos, allow_shrink),
        text_color, dummy_x, dummy_y, adj_x, adj_y,
        width, fontid, has_alpha, nullptr);

    // Update overlay properties
    over.SetImage(std::move(image), has_alpha, adj_x - dummy_x, adj_y - dummy_y);
}

int Overlay_GetX(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.X", scover);
    Point pos = get_overlay_position(*over);
    return game_to_data_coord(pos.X);
}

void Overlay_SetX(ScriptOverlay *scover, int newx)
{
    auto *over = GetOverlayValidate("Overlay.X", scover);
    over->SetPosition(data_to_game_coord(newx), over->GetY());
}

int Overlay_GetY(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Y", scover);
    Point pos = get_overlay_position(*over);
    return game_to_data_coord(pos.Y);
}

void Overlay_SetY(ScriptOverlay *scover, int newy)
{
    auto *over = GetOverlayValidate("Overlay.Y", scover);
    over->SetPosition(over->GetX(), data_to_game_coord(newy));
}

int Overlay_GetGraphic(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Graphic", scover);
    return over->GetSpriteNum();
}

void Overlay_SetGraphic(ScriptOverlay *scover, int slot)
{
    auto *over = GetOverlayValidate("Overlay.Graphic", scover);
    if (!spriteset.DoesSpriteExist(slot))
    {
        debug_script_warn("Overlay.SetGraphic: sprite %d is invalid", slot);
        slot = 0;
    }
    over->SetSpriteNum(slot);
}

bool Overlay_InRoom(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.InRoom", scover);
    return over->IsRoomLayer();
}

int Overlay_GetWidth(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Width", scover);
    return game_to_data_coord(over->GetScaledWidth());
}

int Overlay_GetHeight(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Height", scover);
    return game_to_data_coord(over->GetScaledHeight());
}

int Overlay_GetGraphicWidth(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.GraphicWidth", scover);
    return game_to_data_coord(over->GetGraphicSize().Width);
}

int Overlay_GetGraphicHeight(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.GraphicHeight", scover);
    return game_to_data_coord(over->GetGraphicSize().Height);
}

void Overlay_SetWidth(ScriptOverlay *scover, int width)
{
    auto *over = GetOverlayValidate("Overlay.Width", scover);
    if (width > 0)
        over->SetScaledSize(data_to_game_coord(width), over->GetScaledHeight());
    else
        debug_script_warn("Overlay.SetWidth: invalid width %d", width);
}

void Overlay_SetHeight(ScriptOverlay *scover, int height)
{
    auto *over = GetOverlayValidate("Overlay.Height", scover);
    if (height > 0)
        over->SetScaledSize(over->GetScaledWidth(), data_to_game_coord(height));
    else
        debug_script_warn("Overlay.SetWidth: invalid height %d", height);
}

int Overlay_GetValid(ScriptOverlay *scover)
{
    if (scover->overlayId == -1)
        return 0;

    if (!IsOverlayValid(scover->overlayId))
    {
        scover->overlayId = -1;
        return 0;
    }

    return 1;
}

ScreenOverlay *Overlay_CreateGraphicCore(bool room_layer, int x, int y, int slot, bool transparent, bool clone)
{
    if (!spriteset.DoesSpriteExist(slot))
    {
        debug_script_warn("Overlay.CreateGraphical: sprite %d is invalid", slot);
        return nullptr;
    }

    data_to_game_coords(&x, &y);
    size_t overid;
    // We clone only dynamic sprites, because it makes no sense to clone normal ones
    if (clone && ((game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC) != 0))
    {
        std::unique_ptr<Bitmap> screeno(BitmapHelper::CreateTransparentBitmap(game.SpriteInfos[slot].Width, game.SpriteInfos[slot].Height, game.GetColorDepth()));
        screeno->Blit(spriteset[slot], 0, 0, transparent ? kBitmap_Transparency : kBitmap_Copy);
        overid = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, std::move(screeno),
            (game.SpriteInfos[slot].Flags & SPF_ALPHACHANNEL) != 0);
    }
    else
    {
        overid = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, slot);
    }
    return overid < SIZE_MAX ? &screenover[overid] : nullptr;
}

ScreenOverlay *Overlay_CreateTextCore(bool room_layer, int x, int y, int width, int font, int text_color,
    const char *text, int over_type, DisplayTextStyle style, DisplayTextShrink allow_shrink)
{
    // NOTE: this was not documented, but apparently passing x < 0 or y < 0
    // to Overlay.CreateTextual actually made it centered on screen
    int text_pos = get_textpos_from_scriptcoords(x, y, false);
    if (width < 8) width = play.GetUIViewport().GetWidth() / 2;
    if (text_color == 0) text_color = 16;
    // Skip a voice-over token, if present
    const char *draw_text = skip_voiceover_token(text);
    return display_main(x, y, width, draw_text, nullptr, kDisplayText_NormalOverlay, over_type,
        DisplayTextLooks(style, (DisplayTextPosition)text_pos, allow_shrink),
        font, text_color, false /* no fixed pos */, room_layer);
}

ScriptOverlay* Overlay_CreateGraphicalImpl(bool room_layer, int x, int y, int slot, bool transparent, bool clone)
{
    auto *over = Overlay_CreateGraphicCore(room_layer, x, y, slot, transparent, clone);
    return over ? over->CreateScriptObject() : nullptr;
}

ScriptOverlay* Overlay_CreateGraphical4(int x, int y, int slot, bool transparent)
{
    return Overlay_CreateGraphical(x, y, slot, transparent, true /* clone */);
}

ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, bool transparent, bool clone)
{
    return Overlay_CreateGraphicalImpl(false, x, y, slot, transparent, clone);
}

ScriptOverlay* Overlay_CreateRoomGraphical(int x, int y, int slot, bool transparent, bool clone)
{
    return Overlay_CreateGraphicalImpl(true, x, y, slot, transparent, clone);
}

ScriptOverlay* Overlay_CreateTextualImpl(bool room_layer, int x, int y, int width, int font, int colour, const char* text)
{
    data_to_game_coords(&x, &y);
    width = data_to_game_coord(width);
    auto *over = Overlay_CreateTextCore(room_layer, x, y, width, font, colour, text, OVER_CUSTOM, kDisplayTextStyle_TextWindow, kDisplayTextShrink_None);
    return over ? over->CreateScriptObject() : nullptr;
}

ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text) {
    return Overlay_CreateTextualImpl(false, x, y, width, font, colour, text);
}

ScriptOverlay* Overlay_CreateRoomTextual(int x, int y, int width, int font, int colour, const char* text) {
    return Overlay_CreateTextualImpl(true, x, y, width, font, colour, text);
}

int Overlay_GetTransparency(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Transparency", scover);
    return GfxDef::LegacyTrans255ToTrans100(over->GetTransparency());
}

void Overlay_SetTransparency(ScriptOverlay *scover, int trans)
{
    auto *over = GetOverlayValidate("Overlay.Transparency", scover);
    if ((trans < 0) | (trans > 100))
        quit("!SetTransparency: transparency value must be between 0 and 100");

    over->SetTransparency(GfxDef::Trans100ToLegacyTrans255(trans));
}

bool Overlay_GetVisible(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Visible", scover);
    return over->IsVisible();
}

void Overlay_SetVisible(ScriptOverlay *scover, bool visible)
{
    auto *over = GetOverlayValidate("Overlay.Visible", scover);
    over->SetVisible(visible);
}

void Overlay_SetPosition(ScriptOverlay *scover, int x, int y, int width, int height)
{
    auto *over = GetOverlayValidate("Overlay.SetPosition", scover);
    over->SetPosition(data_to_game_coord(x), data_to_game_coord(y));
    // width and height are optional here
    if (width > 0 || height > 0)
    {
        width = width > 0 ? data_to_game_coord(width) : over->GetScaledWidth();
        height = height > 0 ? data_to_game_coord(height) : over->GetScaledHeight();
        over->SetScaledSize(width, height);
    }
}

void Overlay_SetSize(ScriptOverlay *scover, int width, int height)
{
    auto *over = GetOverlayValidate("Overlay.SetSize", scover);
    if (width > 0 && height > 0)
        over->SetScaledSize(data_to_game_coord(width), data_to_game_coord(height));
    else
        debug_script_warn("Overlay.SetSize: invalid dimensions: %d x %d", width, height);
}

int Overlay_GetZOrder(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.ZOrder", scover);
    return over->GetZOrder();
}

void Overlay_SetZOrder(ScriptOverlay *scover, int zorder)
{
    auto *over = GetOverlayValidate("Overlay.ZOrder", scover);
    over->SetZOrder(zorder);
}

//=============================================================================

void remove_screen_overlay(int type)
{
    if (type < 0 || static_cast<uint32_t>(type) >= screenover.size() || screenover[type].GetID() < 0)
        return; // requested non-existing overlay

    ScreenOverlay &over = screenover[type];
    // Dispose overlay's resources and try dispose a script object (if one exists)
    over.OnRemove();
    // Do necessary changes to the game state after overlay's removal
    // TODO: move these custom settings outside of this function
    // TODO: consider using a callback function ptr, run when overlay is removed
    if (over.GetID() == play.complete_overlay_on)
    {
        play.complete_overlay_on = 0;
    }
    else if (over.GetID() == play.text_overlay_on)
    {
        // reset ref for speech text
        play.speech_text_schandle = ccRemoveObjectHandle(play.speech_text_schandle);
        play.text_overlay_on = 0;
    }
    else if (over.GetID() == OVER_PICTURE)
    {
        // reset ref for speech face
        play.speech_face_schandle = ccRemoveObjectHandle(play.speech_face_schandle);
        face_talking = -1;
    }
    else if (over.GetCharacterRef() >= 0)
    {
        // release internal ref for bg speech
        ccReleaseObjectReference(over.GetScriptHandle());
    }

    // Don't erase vector elements, instead set invalid and record free index
    screenover[type] = ScreenOverlay();
    if (type >= OVER_FIRSTFREE)
        over_free_ids.push(type);

    reset_drawobj_for_overlay(type);

    play.overlay_count--;
    // If all overlays have been removed, reset dynamic draw index (helps vs overflows);
    // we do this here, because overlays are the only dynamic objects atm
    if (play.overlay_count == 0)
        reset_drawobj_dynamic_index();
}

void remove_all_overlays()
{
    for (auto &over : screenover)
        remove_screen_overlay(over.GetID());
}

ScreenOverlay *get_overlay(int type)
{
    return (type >= 0 && static_cast<uint32_t>(type) < screenover.size() &&
        screenover[type].GetID() >= 0) ? &screenover[type] : nullptr;
}

size_t add_screen_overlay_impl(bool roomlayer, int x, int y, int type, int sprnum,
    std::unique_ptr<Bitmap> piccy, int pic_offx, int pic_offy, bool has_alpha)
{
    if (type == OVER_CUSTOM)
    {
        // Find a free ID
        if (over_free_ids.size() > 0)
        {
            type = over_free_ids.front();
            over_free_ids.pop();
        }
        else
        {
            type = std::max(static_cast<size_t>(OVER_FIRSTFREE), screenover.size());
        }
    }

    if (screenover.size() <= static_cast<uint32_t>(type))
        screenover.resize(type + 1);

    ScreenOverlay over(type);
    if (piccy)
    {
        over.SetImage(std::move(piccy), has_alpha, pic_offx, pic_offy);
    }
    else
    {
        over.SetSpriteNum(sprnum, pic_offx, pic_offy);
    }
    over.SetPosition(x, y);
    // by default draw speech and portraits over GUI, and the rest under GUI
    over.SetZOrder((roomlayer || type == OVER_TEXTMSG || type == OVER_PICTURE || type == OVER_TEXTSPEECH) ?
        INT_MAX : INT_MIN);
    over.SetRoomLayer(roomlayer);
    // TODO: move these custom settings outside of this function
    if (type == OVER_COMPLETE)
    {
        play.complete_overlay_on = type;
    }
    else if (type == OVER_TEXTMSG || type == OVER_TEXTSPEECH)
    {
        play.text_overlay_on = type;
        // only make script object for blocking speech now, because messagebox blocks all script
        // and therefore cannot be accessed, so no practical reason for that atm
        if (type == OVER_TEXTSPEECH)
        {
            play.speech_text_schandle = ccAssignObjectHandle(over.CreateScriptObject());
        }
    }
    else if (type == OVER_PICTURE)
    {
        play.speech_face_schandle = ccAssignObjectHandle(over.CreateScriptObject());
    }
    over.MarkChanged();
    screenover[type] = std::move(over);
    play.overlay_count++;
    add_drawobj_for_overlay(type);
    return type;
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, int sprnum)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, sprnum, nullptr, 0, 0, false);
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, std::unique_ptr<Bitmap> piccy, bool has_alpha)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, -1, std::move(piccy), 0, 0, has_alpha);
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, std::unique_ptr<Bitmap> piccy, int pic_offx, int pic_offy, bool has_alpha)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, -1, std::move(piccy), pic_offx, pic_offy, has_alpha);
}

Point get_overlay_position(const ScreenOverlay &over)
{
    if (over.IsRoomLayer())
    {
        return Point(over.GetDrawX(), over.GetDrawY());
    }

    if (over.GetX() == OVR_AUTOPLACE)
    {
        const Rect &ui_view = play.GetUIViewport();
        // auto place on character
        int charid = over.GetY();

        auto view = FindNearestViewport(charid);
        const int charpic = views[game.chars[charid].view].loops[game.chars[charid].loop].frames[0].pic;
        const int height = (charextra[charid].height < 1) ? game.SpriteInfos[charpic].Height : charextra[charid].height;
        const Point screenpt = view->RoomToScreen(
            data_to_game_coord(game.chars[charid].x),
            data_to_game_coord(charextra[charid].GetEffectiveY(&game.chars[charid])) - height).first;
        const Size pic_size = over.GetGraphicSize();
        int tdxp = std::max(0, screenpt.X - pic_size.Width / 2);
        int tdyp = screenpt.Y - get_fixed_pixel_size(5);
        tdyp -= pic_size.Height;
        tdyp = std::max(5, tdyp);

        if ((tdxp + pic_size.Width) >= ui_view.GetWidth())
            tdxp = (ui_view.GetWidth() - pic_size.Width) - 1;
        if (game.chars[charid].room != displayed_room) {
            tdxp = ui_view.GetWidth()/2 - pic_size.Width / 2;
            tdyp = ui_view.GetHeight()/2 - pic_size.Height / 2;
        }
        return Point(tdxp, tdyp);
    }
    else
    {
        // Note: the internal offset is only needed when x,y coordinates are specified
        // and only in the case where the overlay is using a GUI. See issue #1098
        int tdxp = over.GetDrawX();
        int tdyp = over.GetDrawY();
        if (over.IsRoomRelative())
            return play.RoomToScreen(tdxp, tdyp);
        return Point(tdxp, tdyp);
    }
}

void restore_overlays()
{
    // Will have to readjust free ids records, as overlays may be restored in any random slots
    while (!over_free_ids.empty()) { over_free_ids.pop(); }
    for (size_t i = 0; i < screenover.size(); ++i)
    {
        auto &over = screenover[i];
        if (over.GetID() >= 0)
        {
            over.MarkChanged(); // force recreate texture on next draw
        }
        else if (i >= OVER_FIRSTFREE)
        {
            over_free_ids.push(i);
        }
    }
}

std::vector<ScreenOverlay> &get_overlays()
{
    return screenover;
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// ScriptOverlay* (int x, int y, int slot, int transparent)
RuntimeScriptValue Sc_Overlay_CreateGraphical4(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptOverlay, Overlay_CreateGraphical4);
}

RuntimeScriptValue Sc_Overlay_CreateGraphical(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL2(ScriptOverlay, Overlay_CreateGraphical);
}

RuntimeScriptValue Sc_Overlay_CreateRoomGraphical(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL2(ScriptOverlay, Overlay_CreateRoomGraphical);
}

// ScriptOverlay* (int x, int y, int width, int font, int colour, const char* text, ...)
RuntimeScriptValue Sc_Overlay_CreateTextual(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(Overlay_CreateTextual, 6);
    ScriptOverlay *overlay = Overlay_CreateTextual(params[0].IValue, params[1].IValue, params[2].IValue,
                                                   params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue().SetScriptObject(overlay, overlay);
}

RuntimeScriptValue Sc_Overlay_CreateRoomTextual(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(Overlay_CreateRoomTextual, 6);
    ScriptOverlay *overlay = Overlay_CreateRoomTextual(params[0].IValue, params[1].IValue, params[2].IValue,
        params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue().SetScriptObject(overlay, overlay);
}

// void (ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...)
RuntimeScriptValue Sc_Overlay_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Overlay_SetText, 4);
    Overlay_SetText((ScriptOverlay*)self, params[0].IValue, params[1].IValue, params[2].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

// void (ScriptOverlay *sco)
RuntimeScriptValue Sc_Overlay_Remove(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptOverlay, Overlay_Remove);
}

// int (ScriptOverlay *scover)
RuntimeScriptValue Sc_Overlay_GetValid(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetValid);
}

// int (ScriptOverlay *scover)
RuntimeScriptValue Sc_Overlay_GetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetX);
}

// void (ScriptOverlay *scover, int newx)
RuntimeScriptValue Sc_Overlay_SetX(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetX);
}

// int (ScriptOverlay *scover)
RuntimeScriptValue Sc_Overlay_GetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetY);
}

// void (ScriptOverlay *scover, int newy)
RuntimeScriptValue Sc_Overlay_SetY(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetY);
}

RuntimeScriptValue Sc_Overlay_GetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphic);
}

RuntimeScriptValue Sc_Overlay_SetGraphic(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetGraphic);
}

RuntimeScriptValue Sc_Overlay_InRoom(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptOverlay, Overlay_InRoom);
}

RuntimeScriptValue Sc_Overlay_GetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetWidth);
}

RuntimeScriptValue Sc_Overlay_SetWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetWidth);
}

RuntimeScriptValue Sc_Overlay_GetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetHeight);
}

RuntimeScriptValue Sc_Overlay_SetHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetHeight);
}

RuntimeScriptValue Sc_Overlay_GetGraphicWidth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphicWidth);
}

RuntimeScriptValue Sc_Overlay_GetGraphicHeight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetGraphicHeight);
}

RuntimeScriptValue Sc_Overlay_GetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTransparency);
}

RuntimeScriptValue Sc_Overlay_SetTransparency(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetTransparency);
}

RuntimeScriptValue Sc_Overlay_GetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptOverlay, Overlay_GetVisible);
}

RuntimeScriptValue Sc_Overlay_SetVisible(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PBOOL(ScriptOverlay, Overlay_SetVisible);
}

RuntimeScriptValue Sc_Overlay_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetZOrder);
}

RuntimeScriptValue Sc_Overlay_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetZOrder);
}

RuntimeScriptValue Sc_Overlay_SetPosition(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT4(ScriptOverlay, Overlay_SetPosition);
}

RuntimeScriptValue Sc_Overlay_SetSize(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT2(ScriptOverlay, Overlay_SetSize);
}


//=============================================================================
//
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

// ScriptOverlay* (int x, int y, int width, int font, int colour, const char* text, ...)
ScriptOverlay* ScPl_Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    return Overlay_CreateTextual(x, y, width, font, colour, scsf_buffer);
}

ScriptOverlay* ScPl_Overlay_CreateRoomTextual(int x, int y, int width, int font, int colour, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    return Overlay_CreateRoomTextual(x, y, width, font, colour, scsf_buffer);
}

// void (ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...)
void ScPl_Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int clr, char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    Overlay_SetText(scover, wii, fontid, clr, scsf_buffer);
}

void RegisterOverlayAPI()
{
    ScFnRegister overlay_api[] = {
        { "Overlay::CreateGraphical^4",   API_FN_PAIR(Overlay_CreateGraphical4) },
        { "Overlay::CreateGraphical^5",   API_FN_PAIR(Overlay_CreateGraphical) },
        { "Overlay::CreateTextual^106",   Sc_Overlay_CreateTextual, ScPl_Overlay_CreateTextual },
        { "Overlay::CreateRoomGraphical^5", API_FN_PAIR(Overlay_CreateRoomGraphical) },
        { "Overlay::CreateRoomTextual^106", Sc_Overlay_CreateRoomTextual, ScPl_Overlay_CreateRoomTextual },

        { "Overlay::SetText^104",         Sc_Overlay_SetText, ScPl_Overlay_SetText },
        { "Overlay::Remove^0",            API_FN_PAIR(Overlay_Remove) },
        { "Overlay::SetPosition^4",       API_FN_PAIR(Overlay_SetPosition) },
        { "Overlay::SetSize^2",           API_FN_PAIR(Overlay_SetSize) },
        { "Overlay::get_Valid",           API_FN_PAIR(Overlay_GetValid) },
        { "Overlay::get_X",               API_FN_PAIR(Overlay_GetX) },
        { "Overlay::set_X",               API_FN_PAIR(Overlay_SetX) },
        { "Overlay::get_Y",               API_FN_PAIR(Overlay_GetY) },
        { "Overlay::set_Y",               API_FN_PAIR(Overlay_SetY) },
        { "Overlay::get_Graphic",         API_FN_PAIR(Overlay_GetGraphic) },
        { "Overlay::set_Graphic",         API_FN_PAIR(Overlay_SetGraphic) },
        { "Overlay::get_InRoom",          API_FN_PAIR(Overlay_InRoom) },
        { "Overlay::get_Width",           API_FN_PAIR(Overlay_GetWidth) },
        { "Overlay::set_Width",           API_FN_PAIR(Overlay_SetWidth) },
        { "Overlay::get_Height",          API_FN_PAIR(Overlay_GetHeight) },
        { "Overlay::set_Height",          API_FN_PAIR(Overlay_SetHeight) },
        { "Overlay::get_GraphicWidth",    API_FN_PAIR(Overlay_GetGraphicWidth) },
        { "Overlay::get_GraphicHeight",   API_FN_PAIR(Overlay_GetGraphicHeight) },
        { "Overlay::get_Transparency",    API_FN_PAIR(Overlay_GetTransparency) },
        { "Overlay::set_Transparency",    API_FN_PAIR(Overlay_SetTransparency) },
        { "Overlay::get_Visible",         API_FN_PAIR(Overlay_GetVisible) },
        { "Overlay::set_Visible",         API_FN_PAIR(Overlay_SetVisible) },
        { "Overlay::get_ZOrder",          API_FN_PAIR(Overlay_GetZOrder) },
        { "Overlay::set_ZOrder",          API_FN_PAIR(Overlay_SetZOrder) },
    };

    ccAddExternalFunctions(overlay_api);
}
