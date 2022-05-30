//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================
#include "ac/overlay.h"
#include <algorithm>
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
#include "debug/debug_log.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern int displayed_room;
extern int face_talking;
extern std::vector<ViewStruct> views;
extern IGraphicsDriver *gfxDriver;


std::vector<ScreenOverlay> screenover;

void Overlay_Remove(ScriptOverlay *sco) {
    sco->Remove();
}

void Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int text_color, const char *text) {
    int ovri=find_overlay_of_type(scover->overlayId);
    if (ovri<0)
        quit("!Overlay.SetText: invalid overlay ID specified");
    int xx = game_to_data_coord(screenover[ovri].x);
    int yy = game_to_data_coord(screenover[ovri].y);

    RemoveOverlay(scover->overlayId);
    const int disp_type = scover->overlayId;

    if (CreateTextOverlay(xx, yy, wii, fontid, text_color, get_translation(text), disp_type) != scover->overlayId)
        quit("SetTextOverlay internal error: inconsistent type ids");
}

int Overlay_GetX(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    Point pos = get_overlay_position(screenover[ovri]);
    return game_to_data_coord(pos.X);
}

void Overlay_SetX(ScriptOverlay *scover, int newx) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    screenover[ovri].x = data_to_game_coord(newx);
}

int Overlay_GetY(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    Point pos = get_overlay_position(screenover[ovri]);
    return game_to_data_coord(pos.Y);
}

void Overlay_SetY(ScriptOverlay *scover, int newy) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    screenover[ovri].y = data_to_game_coord(newy);
}

int Overlay_GetGraphic(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    return screenover[ovri].GetSpriteNum();
}

void Overlay_SetGraphic(ScriptOverlay *scover, int slot) {
    if (!spriteset.DoesSpriteExist(slot))
    {
        debug_script_warn("Overlay.SetGraphic: sprite %d is invalid", slot);
        slot = 0;
    }
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    screenover[ovri].SetSpriteNum(slot);
}

bool Overlay_InRoom(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    return screenover[ovri].IsRoomLayer();
}

int Overlay_GetWidth(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    return game_to_data_coord(screenover[ovri].scaleWidth);
}

int Overlay_GetHeight(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    return game_to_data_coord(screenover[ovri].scaleHeight);
}

int Overlay_GetGraphicWidth(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    return game_to_data_coord(screenover[ovri].GetImage()->GetWidth());
}

int Overlay_GetGraphicHeight(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    return game_to_data_coord(screenover[ovri].GetImage()->GetHeight());
}

void Overlay_SetScaledSize(ScreenOverlay &over, int width, int height) {
    data_to_game_coords(&width, &height);
    if (width < 1 || height < 1)
    {
        debug_script_warn("Overlay.SetSize: invalid dimensions: %d x %d", width, height);
        return;
    }
    if ((width == over.scaleWidth) && (height == over.scaleHeight))
        return; // no change
    over.scaleWidth = width;
    over.scaleHeight = height;
    over.MarkChanged();
}

void Overlay_SetWidth(ScriptOverlay *scover, int width) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    Overlay_SetScaledSize(screenover[ovri], width, game_to_data_coord(screenover[ovri].scaleHeight));
}

void Overlay_SetHeight(ScriptOverlay *scover, int height) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    Overlay_SetScaledSize(screenover[ovri], game_to_data_coord(screenover[ovri].scaleWidth), height);
}

int Overlay_GetValid(ScriptOverlay *scover) {
    if (scover->overlayId == -1)
        return 0;

    if (!IsOverlayValid(scover->overlayId)) {
        scover->overlayId = -1;
        return 0;
    }

    return 1;
}

ScreenOverlay *Overlay_CreateGraphicCore(bool room_layer, int x, int y, int slot, bool transparent, bool clone)
{
    data_to_game_coords(&x, &y);
    size_t overid;
    // We clone only dynamic sprites, because it makes no sense to clone normal ones
    if (clone && ((game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC) != 0))
    {
        Bitmap *screeno = BitmapHelper::CreateTransparentBitmap(game.SpriteInfos[slot].Width, game.SpriteInfos[slot].Height, game.GetColorDepth());
        screeno->Blit(spriteset[slot], 0, 0, transparent ? kBitmap_Transparency : kBitmap_Copy);
        overid = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, screeno,
            (game.SpriteInfos[slot].Flags & SPF_ALPHACHANNEL) != 0);
    }
    else
    {
        overid = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, slot);
    }
    return overid < SIZE_MAX ? &screenover[overid] : nullptr;
}

ScreenOverlay *Overlay_CreateTextCore(bool room_layer, int x, int y, int width, int font, int text_color,
    const char *text, int disp_type, int allow_shrink)
{
    if (width < 8) width = play.GetUIViewport().GetWidth() / 2;
    if (x < 0) x = play.GetUIViewport().GetWidth() / 2 - width / 2;
    if (text_color == 0) text_color = 16;
    return _display_main(x, y, width, text, disp_type, font, -text_color, 0, allow_shrink, false, room_layer);
}

ScriptOverlay* Overlay_CreateGraphicalEx(bool room_layer, int x, int y, int slot, int transparent, bool clone)
{
    auto *over = Overlay_CreateGraphicCore(room_layer, x, y, slot, transparent != 0, clone);
    return over ? create_scriptoverlay(*over) : nullptr;
}

ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, int transparent)
{
    auto *over = Overlay_CreateGraphicCore(false, x, y, slot, transparent != 0, true); // always clone
    return over ? create_scriptoverlay(*over) : nullptr;
}

ScriptOverlay* Overlay_CreateTextualEx(bool room_layer, int x, int y, int width, int font, int colour, const char* text)
{
    data_to_game_coords(&x, &y);
    width = data_to_game_coord(width);
    auto *over = Overlay_CreateTextCore(room_layer, x, y, width, font, colour, text, DISPLAYTEXT_NORMALOVERLAY, 0);
    return over ? create_scriptoverlay(*over) : nullptr;
}

ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text) {
    return Overlay_CreateTextualEx(false, x, y, width, font, colour, text);
}

int Overlay_GetTransparency(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    return GfxDef::LegacyTrans255ToTrans100(screenover[ovri].transparency);
}

void Overlay_SetTransparency(ScriptOverlay *scover, int trans) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");
    if ((trans < 0) | (trans > 100))
        quit("!SetTransparency: transparency value must be between 0 and 100");

    screenover[ovri].transparency = GfxDef::Trans100ToLegacyTrans255(trans);
}

int Overlay_GetZOrder(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    return screenover[ovri].zorder;
}

void Overlay_SetZOrder(ScriptOverlay *scover, int zorder) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    screenover[ovri].zorder = zorder;
}

//=============================================================================

// Creates and registers a managed script object for existing overlay object;
// optionally adds an internal engine reference to prevent object's disposal
ScriptOverlay* create_scriptoverlay(ScreenOverlay &over, bool internal_ref)
{
    ScriptOverlay *scover = new ScriptOverlay();
    scover->overlayId = over.type;
    int handl = ccRegisterManagedObject(scover, scover);
    over.associatedOverlayHandle = handl;
    if (internal_ref)
        ccAddObjectReference(handl);
    return scover;
}

// Invalidates existing script object to let user know that previous overlay is gone,
// and releases engine's internal reference (script object may exist while there are user refs)
static void invalidate_and_subref(ScreenOverlay &over, ScriptOverlay **scover)
{
    if (scover && (*scover))
    {
        (*scover)->overlayId = -1;
        *scover = nullptr;
    }
    else if (over.associatedOverlayHandle > 0)
    {
        ScriptOverlay *scover = (ScriptOverlay*)ccGetObjectAddressFromHandle(over.associatedOverlayHandle);
        if (scover) scover->overlayId = -1;
    }

    if (over.associatedOverlayHandle > 0)
    {
        ccReleaseObjectReference(over.associatedOverlayHandle);
        over.associatedOverlayHandle = 0;
    }
}

// Frees overlay resources and tell to dispose script object if there are no refs left
static void dispose_overlay(ScreenOverlay &over)
{
    over.SetImage(nullptr);
    if (over.ddb != nullptr)
        gfxDriver->DestroyDDB(over.ddb);
    over.ddb = nullptr;
    // invalidate script object and dispose it if there are no more refs
    if (over.associatedOverlayHandle > 0)
    {
        ScriptOverlay *scover = (ScriptOverlay*)ccGetObjectAddressFromHandle(over.associatedOverlayHandle);
        if (scover) scover->overlayId = -1;
        ccAttemptDisposeObject(over.associatedOverlayHandle);
    }
}

void remove_screen_overlay_index(size_t over_idx)
{
    assert(over_idx < screenover.size());
    if (over_idx >= screenover.size())
        return; // something is wrong
    ScreenOverlay &over = screenover[over_idx];
    // TODO: move these custom settings outside of this function
    if (over.type == play.complete_overlay_on)
    {
        play.complete_overlay_on = 0;
    }
    else if (over.type == play.text_overlay_on)
    { // release internal ref for speech text
        invalidate_and_subref(over, &play.speech_text_scover);
        play.text_overlay_on = 0;
    }
    else if (over.type == OVER_PICTURE)
    { // release internal ref for speech face
        invalidate_and_subref(over, &play.speech_face_scover);
        face_talking = -1;
    }
    else if (over.bgSpeechForChar >= 0)
    { // release internal ref for bg speech
        invalidate_and_subref(over, nullptr);
    }
    dispose_overlay(over);
    screenover.erase(screenover.begin() + over_idx);
    // if an overlay before the sierra-style speech one is removed, update the index
    // TODO: this is bad, need more generic system to store overlay references
    if ((size_t)face_talking > over_idx)
        face_talking--;
}

void remove_screen_overlay(int type)
{
    for (size_t i = 0; i < screenover.size();)
    {
        if (type < 0 || screenover[i].type == type)
            remove_screen_overlay_index(i);
        else
            i++;
    }
}

int find_overlay_of_type(int type)
{
    for (size_t i = 0; i < screenover.size(); ++i)
    {
        if (screenover[i].type == type) return i;
    }
    return -1;
}

size_t add_screen_overlay_impl(bool roomlayer, int x, int y, int type, int sprnum, Bitmap *piccy,
    int pic_offx, int pic_offy, bool has_alpha)
{
    if (type == OVER_CUSTOM) {
        // find an unused custom ID; TODO: find a better approach!
        for (int id = OVER_CUSTOM + 1; (size_t)id <= screenover.size() + OVER_CUSTOM + 1; ++id) {
            if (find_overlay_of_type(id) == -1) { type=id; break; }
        }
    }
    ScreenOverlay over;
    if (piccy)
    {
        over.SetImage(std::unique_ptr<Bitmap>(piccy));
        over.SetAlphaChannel(has_alpha);
    }
    else
    {
        over.SetSpriteNum(sprnum);
        over.SetAlphaChannel((game.SpriteInfos[sprnum].Flags & SPF_ALPHACHANNEL) != 0);
    }
    over.ddb = nullptr; // is generated during first draw pass
    over.x=x;
    over.y=y;
    over.offsetX = pic_offx;
    over.offsetY = pic_offy;
    // by default draw speech and portraits over GUI, and the rest under GUI
    over.zorder = (roomlayer || type == OVER_TEXTMSG || type == OVER_PICTURE || type == OVER_TEXTSPEECH) ?
        INT_MAX : INT_MIN;
    over.type=type;
    over.timeout=0;
    over.bgSpeechForChar = -1;
    over.associatedOverlayHandle = 0;
    over.SetRoomLayer(roomlayer);
    // TODO: move these custom settings outside of this function
    if (type == OVER_COMPLETE) play.complete_overlay_on = type;
    else if (type == OVER_TEXTMSG || type == OVER_TEXTSPEECH)
    {
        play.text_overlay_on = type;
        // only make script object for blocking speech now, because messagebox blocks all script
        // and therefore cannot be accessed, so no practical reason for that atm
        if (type == OVER_TEXTSPEECH)
            play.speech_text_scover = create_scriptoverlay(over, true);
    }
    else if (type == OVER_PICTURE)
    {
        play.speech_face_scover = create_scriptoverlay(over, true);
    }
    over.MarkChanged();
    screenover.push_back(std::move(over));
    return screenover.size() - 1;
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, int sprnum)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, sprnum, nullptr, 0, 0, false);
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, Bitmap *piccy, bool has_alpha)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, -1, piccy, 0, 0, has_alpha);
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, Common::Bitmap *piccy, int pic_offx, int pic_offy, bool has_alpha)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, -1, piccy, pic_offx, pic_offy, has_alpha);
}

Point get_overlay_position(const ScreenOverlay &over)
{
    if (over.IsRoomLayer())
    {
        return Point(over.x + over.offsetX, over.y + over.offsetY);
    }

    if (over.x == OVR_AUTOPLACE)
    {
        const Rect &ui_view = play.GetUIViewport();
        // auto place on character
        int charid = over.y;

        auto view = FindNearestViewport(charid);
        const int charpic = views[game.chars[charid].view].loops[game.chars[charid].loop].frames[0].pic;
        const int height = (charextra[charid].height < 1) ? game.SpriteInfos[charpic].Height : charextra[charid].height;
        Point screenpt = view->RoomToScreen(
            data_to_game_coord(game.chars[charid].x),
            data_to_game_coord(game.chars[charid].get_effective_y()) - height).first;
        Bitmap *pic = over.GetImage();
        int tdxp = std::max(0, screenpt.X - pic->GetWidth() / 2);
        int tdyp = screenpt.Y - get_fixed_pixel_size(5);
        tdyp -= pic->GetHeight();
        tdyp = std::max(5, tdyp);

        if ((tdxp + pic->GetWidth()) >= ui_view.GetWidth())
            tdxp = (ui_view.GetWidth() - pic->GetWidth()) - 1;
        if (game.chars[charid].room != displayed_room) {
            tdxp = ui_view.GetWidth()/2 - pic->GetWidth()/2;
            tdyp = ui_view.GetHeight()/2 - pic->GetHeight()/2;
        }
        return Point(tdxp, tdyp);
    }
    else
    {
        // Note: the internal offset is only needed when x,y coordinates are specified
        // and only in the case where the overlay is using a GUI. See issue #1098
        int tdxp = over.x + over.offsetX;
        int tdyp = over.y + over.offsetY;
        if (over.IsRoomRelative())
            return play.RoomToScreen(tdxp, tdyp);
        return Point(tdxp, tdyp);
    }
}

void recreate_overlay_ddbs()
{
    for (auto &over : screenover)
    {
        if (over.ddb)
            gfxDriver->DestroyDDB(over.ddb);
        over.ddb = nullptr; // is generated during first draw pass
        over.MarkChanged();
    }
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
RuntimeScriptValue Sc_Overlay_CreateGraphical(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT(FUNCTION, 4);
    ScriptOverlay *overlay = Overlay_CreateGraphicalEx(false, params[0].IValue, params[1].IValue, params[2].IValue,
        params[3].IValue, true); // always clone image
    return RuntimeScriptValue().SetDynamicObject(overlay, overlay);
}

RuntimeScriptValue Sc_Overlay_CreateGraphicalRef(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT(FUNCTION, 5);
    ScriptOverlay *overlay = Overlay_CreateGraphicalEx(false, params[0].IValue, params[1].IValue, params[2].IValue,
        params[3].IValue, params[4].GetAsBool());
    return RuntimeScriptValue().SetDynamicObject(overlay, overlay);
}

RuntimeScriptValue Sc_Overlay_CreateRoomGraphical(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT(FUNCTION, 5);
    ScriptOverlay *overlay = Overlay_CreateGraphicalEx(true, params[0].IValue, params[1].IValue, params[2].IValue,
        params[3].IValue, params[4].GetAsBool());
    return RuntimeScriptValue().SetDynamicObject(overlay, overlay);
}

// ScriptOverlay* (int x, int y, int width, int font, int colour, const char* text, ...)
RuntimeScriptValue Sc_Overlay_CreateTextual(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(Overlay_CreateTextual, 6);
    ScriptOverlay *overlay = Overlay_CreateTextualEx(false, params[0].IValue, params[1].IValue, params[2].IValue,
                                                   params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue().SetDynamicObject(overlay, overlay);
}

RuntimeScriptValue Sc_Overlay_CreateRoomTextual(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(Overlay_CreateRoomTextual, 6);
    ScriptOverlay *overlay = Overlay_CreateTextualEx(true, params[0].IValue, params[1].IValue, params[2].IValue,
        params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue().SetDynamicObject(overlay, overlay);
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

RuntimeScriptValue Sc_Overlay_GetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetZOrder);
}

RuntimeScriptValue Sc_Overlay_SetZOrder(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetZOrder);
}


//=============================================================================
//
// Exclusive API for Plugins
//
//=============================================================================

// ScriptOverlay* (int x, int y, int width, int font, int colour, const char* text, ...)
ScriptOverlay* ScPl_Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char *text, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(text);
    return Overlay_CreateTextual(x, y, width, font, colour, scsf_buffer);
}

// void (ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...)
void ScPl_Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int clr, char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    Overlay_SetText(scover, wii, fontid, clr, scsf_buffer);
}

void RegisterOverlayAPI()
{
    ccAddExternalStaticFunction("Overlay::CreateGraphical^4",   Sc_Overlay_CreateGraphical);
    ccAddExternalStaticFunction("Overlay::CreateGraphical^5",   Sc_Overlay_CreateGraphicalRef);
    ccAddExternalStaticFunction("Overlay::CreateTextual^106",   Sc_Overlay_CreateTextual);
    ccAddExternalStaticFunction("Overlay::CreateRoomGraphical^5", Sc_Overlay_CreateRoomGraphical);
    ccAddExternalStaticFunction("Overlay::CreateRoomTextual^106", Sc_Overlay_CreateRoomTextual);
    ccAddExternalObjectFunction("Overlay::SetText^104",         Sc_Overlay_SetText);
    ccAddExternalObjectFunction("Overlay::Remove^0",            Sc_Overlay_Remove);
    ccAddExternalObjectFunction("Overlay::get_Valid",           Sc_Overlay_GetValid);
    ccAddExternalObjectFunction("Overlay::get_X",               Sc_Overlay_GetX);
    ccAddExternalObjectFunction("Overlay::set_X",               Sc_Overlay_SetX);
    ccAddExternalObjectFunction("Overlay::get_Y",               Sc_Overlay_GetY);
    ccAddExternalObjectFunction("Overlay::set_Y",               Sc_Overlay_SetY);
    ccAddExternalObjectFunction("Overlay::get_Graphic",         Sc_Overlay_GetGraphic);
    ccAddExternalObjectFunction("Overlay::set_Graphic",         Sc_Overlay_SetGraphic);
    ccAddExternalObjectFunction("Overlay::get_InRoom",          Sc_Overlay_InRoom);
    ccAddExternalObjectFunction("Overlay::get_Width",           Sc_Overlay_GetWidth);
    ccAddExternalObjectFunction("Overlay::set_Width",           Sc_Overlay_SetWidth);
    ccAddExternalObjectFunction("Overlay::get_Height",          Sc_Overlay_GetHeight);
    ccAddExternalObjectFunction("Overlay::set_Height",          Sc_Overlay_SetHeight);
    ccAddExternalObjectFunction("Overlay::get_GraphicWidth",    Sc_Overlay_GetGraphicWidth);
    ccAddExternalObjectFunction("Overlay::get_GraphicHeight",   Sc_Overlay_GetGraphicHeight);
    ccAddExternalObjectFunction("Overlay::get_Transparency",    Sc_Overlay_GetTransparency);
    ccAddExternalObjectFunction("Overlay::set_Transparency",    Sc_Overlay_SetTransparency);
    ccAddExternalObjectFunction("Overlay::get_ZOrder",          Sc_Overlay_GetZOrder);
    ccAddExternalObjectFunction("Overlay::set_ZOrder",          Sc_Overlay_SetZOrder);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("Overlay::CreateGraphical^4",   (void*)Overlay_CreateGraphical);
    ccAddExternalFunctionForPlugin("Overlay::CreateTextual^106",   (void*)ScPl_Overlay_CreateTextual);
    ccAddExternalFunctionForPlugin("Overlay::SetText^104",         (void*)ScPl_Overlay_SetText);
    ccAddExternalFunctionForPlugin("Overlay::Remove^0",            (void*)Overlay_Remove);
    ccAddExternalFunctionForPlugin("Overlay::get_Valid",           (void*)Overlay_GetValid);
    ccAddExternalFunctionForPlugin("Overlay::get_X",               (void*)Overlay_GetX);
    ccAddExternalFunctionForPlugin("Overlay::set_X",               (void*)Overlay_SetX);
    ccAddExternalFunctionForPlugin("Overlay::get_Y",               (void*)Overlay_GetY);
    ccAddExternalFunctionForPlugin("Overlay::set_Y",               (void*)Overlay_SetY);
}
