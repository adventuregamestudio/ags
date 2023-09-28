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
#include <queue>
#include "ac/common.h"
#include "ac/view.h"
#include "ac/character.h"
#include "ac/characterextras.h"
#include "ac/display.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
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
extern int displayed_room;
extern int face_talking;
extern std::vector<ViewStruct> views;
extern IGraphicsDriver *gfxDriver;

// TODO: consider some kind of a "object pool" template,
// which handles this kind of storage; share with ManagedPool's handles?
std::vector<ScreenOverlay> screenover;
std::queue<int32_t> over_free_ids;


void Overlay_Remove(ScriptOverlay *sco) {
    sco->Remove();
}

void Overlay_SetText(ScriptOverlay *scover, int width, int fontid, int text_color, const char *text)
{
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!Overlay.SetText: invalid overlay ID specified");
    const int x = over->x;
    const int y = over->y;

    // TODO: find a nice way to refactor and share these code pieces
    // from CreateTextOverlay
    // allow DisplaySpeechBackground to be shrunk
    int allow_shrink = (x == OVR_AUTOPLACE) ? 1 : 0;

    // from Overlay_CreateTextCore
    if (width < 8) width = play.GetUIViewport().GetWidth() / 2;
    if (text_color == 0) text_color = 16;

    // Recreate overlay image
    int dummy_x = x, dummy_y = y, adj_x = x, adj_y = y;
    // NOTE: we pass text_color negated to let optionally use textwindow (if applicable)
    // this is a generic ugliness of _display_main args, need to refactor later.
    Bitmap *image = create_textual_image(get_translation(text), -text_color, 0, dummy_x, dummy_y, adj_x, adj_y,
        width, fontid, allow_shrink);

    // Update overlay properties
    over->SetImage(std::unique_ptr<Bitmap>(image), adj_x - dummy_x, adj_y - dummy_y);
}

int Overlay_GetX(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return get_overlay_position(*over).X;
}

void Overlay_SetX(ScriptOverlay *scover, int newx) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    over->x = newx;
}

int Overlay_GetY(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return get_overlay_position(*over).Y;
}

void Overlay_SetY(ScriptOverlay *scover, int newy) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    over->y = newy;
}

int Overlay_GetGraphic(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return over->GetSpriteNum();
}

void Overlay_SetGraphic(ScriptOverlay *scover, int slot) {
    if (!spriteset.DoesSpriteExist(slot))
    {
        debug_script_warn("Overlay.SetGraphic: sprite %d is invalid", slot);
        slot = 0;
    }
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    over->SetSpriteNum(slot);
}

bool Overlay_InRoom(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return over->IsRoomLayer();
}

int Overlay_GetWidth(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return over->scaleWidth;
}

int Overlay_GetHeight(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return over->scaleHeight;
}

int Overlay_GetGraphicWidth(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return over->GetGraphicSize().Width;
}

int Overlay_GetGraphicHeight(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return over->GetGraphicSize().Height;
}

void Overlay_SetScaledSize(ScreenOverlay &over, int width, int height) {
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
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    Overlay_SetScaledSize(*over, width, over->scaleHeight);
}

void Overlay_SetHeight(ScriptOverlay *scover, int height) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    Overlay_SetScaledSize(*over, over->scaleWidth, height);
}

int Overlay_GetValid(ScriptOverlay *scover) {
    return get_overlay(scover->overlayId) != nullptr;
}

ScreenOverlay *Overlay_CreateGraphicCore(bool room_layer, int x, int y, int slot, bool clone)
{
    size_t overid;
    // We clone only dynamic sprites, because it makes no sense to clone normal ones
    if (clone && ((game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC) != 0))
    {
        Bitmap *screeno = new Bitmap(game.SpriteInfos[slot].Width, game.SpriteInfos[slot].Height, game.GetColorDepth());
        screeno->Blit(spriteset[slot], 0, 0);
        overid = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, screeno);
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

ScriptOverlay* Overlay_CreateGraphicalImpl(bool room_layer, int x, int y, int slot, bool clone)
{
    auto *over = Overlay_CreateGraphicCore(room_layer, x, y, slot, clone);
    return over ? create_scriptoverlay(*over) : nullptr;
}

ScriptOverlay* Overlay_CreateTextualImpl(bool room_layer, int x, int y, int width, int font, int colour, const char* text)
{
    auto *over = Overlay_CreateTextCore(room_layer, x, y, width, font, colour, text, DISPLAYTEXT_NORMALOVERLAY, 0);
    return over ? create_scriptoverlay(*over) : nullptr;
}

ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, bool clone)
{
    return Overlay_CreateGraphicalImpl(false, x, y, slot, clone);
}

// This is a fallback for running 3.6.0 games, strictly for regression testing convenience
ScriptOverlay* Overlay_CreateGraphical5(int x, int y, int slot, bool /* dummy */, bool clone)
{
    return Overlay_CreateGraphical(x, y, slot, clone);
}

ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text)
{
    return Overlay_CreateTextualImpl(false, x, y, width, font, colour, text);
}

ScriptOverlay* Overlay_CreateRoomGraphical(int x, int y, int slot, bool clone)
{
    return Overlay_CreateGraphicalImpl(true, x, y, slot, clone);
}

// This is a fallback for running 3.6.0 games, strictly for regression testing convenience
ScriptOverlay* Overlay_CreateRoomGraphical5(int x, int y, int slot, bool /*dummy*/, bool clone)
{
    return Overlay_CreateRoomGraphical(x, y, slot, clone);
}

ScriptOverlay* Overlay_CreateRoomTextual(int x, int y, int width, int font, int colour, const char* text)
{
    return Overlay_CreateTextualImpl(true, x, y, width, font, colour, text);
}

int Overlay_GetBlendMode(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");

    return over->blendMode;
}

void Overlay_SetBlendMode(ScriptOverlay *scover, int blendMode) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    if ((blendMode < 0) || (blendMode >= kNumBlendModes))
        quitprintf("!SetBlendMode: invalid blend mode %d, supported modes are %d - %d", blendMode, 0, kNumBlendModes - 1);
    over->blendMode = (BlendMode)blendMode;
}

int Overlay_GetTransparency(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");

    return GfxDef::LegacyTrans255ToTrans100(over->transparency);
}

void Overlay_SetTransparency(ScriptOverlay *scover, int trans) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    if ((trans < 0) | (trans > 100))
        quit("!SetTransparency: transparency value must be between 0 and 100");

    over->transparency = GfxDef::Trans100ToLegacyTrans255(trans);
}

float Overlay_GetRotation(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    return over->rotation;
}

void Overlay_SetRotation(ScriptOverlay *scover, float degrees) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");
    over->rotation = Math::ClampAngle360(degrees);
}

int Overlay_GetZOrder(ScriptOverlay *scover) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");

    return over->zorder;
}

void Overlay_SetZOrder(ScriptOverlay *scover, int zorder) {
    auto *over = get_overlay(scover->overlayId);
    if (!over)
        quit("!invalid overlay ID specified");

    over->zorder = zorder;
}

//=============================================================================

// Creates and registers a managed script object for existing overlay object;
// optionally adds an internal engine reference to prevent object's disposal
ScriptOverlay* create_scriptoverlay(ScreenOverlay &over, bool internal_ref)
{
    ScriptOverlay *scover = new ScriptOverlay();
    scover->overlayId = over.type;
    int handl = ccRegisterManagedObject(scover, scover);
    over.associatedOverlayHandle = handl; // save the handle for access
    if (internal_ref) // requested additional ref
        ccAddObjectReference(handl);
    return scover;
}

// Invalidates existing script object to let user know that previous overlay is gone,
// and releases engine's internal reference (script object may exist while there are user refs)
static void invalidate_and_subref(ScreenOverlay &over)
{
    if (over.associatedOverlayHandle <= 0)
        return; // invalid handle

    ScriptOverlay *scover = (ScriptOverlay*)ccGetObjectAddressFromHandle(over.associatedOverlayHandle);
    if (scover)
    {
        scover->overlayId = -1; // invalidate script object
        ccReleaseObjectReference(over.associatedOverlayHandle);
    }
    over.associatedOverlayHandle = 0; // reset internal handle
}

// Frees overlay resources and tell to dispose script object if there are no refs left
static void dispose_overlay(ScreenOverlay &over)
{
    over.SetImage(nullptr);
    // invalidate script object and dispose it if there are no more refs
    if (over.associatedOverlayHandle > 0)
    {
        ScriptOverlay *scover = (ScriptOverlay*)ccGetObjectAddressFromHandle(over.associatedOverlayHandle);
        if (scover) scover->overlayId = -1;
        ccAttemptDisposeObject(over.associatedOverlayHandle);
    }
}

void remove_screen_overlay(int type)
{
    if (type < 0 || static_cast<uint32_t>(type) >= screenover.size() || screenover[type].type < 0)
        return; // requested non-existing overlay

    ScreenOverlay &over = screenover[type];
    // TODO: move these custom settings outside of this function
    if (over.type == play.complete_overlay_on)
    {
        play.complete_overlay_on = 0;
    }
    else if (over.type == play.text_overlay_on)
    { // release internal ref for speech text
        invalidate_and_subref(over);
        play.speech_text_schandle = 0;
        play.text_overlay_on = 0;
    }
    else if (over.type == OVER_PICTURE)
    { // release internal ref for speech face
        invalidate_and_subref(over);
        play.speech_face_schandle = 0;
        face_talking = -1;
    }
    else if (over.bgSpeechForChar >= 0)
    { // release internal ref for bg speech
        invalidate_and_subref(over);
    }
    dispose_overlay(over);

    // Don't erase vector elements, instead set invalid and record free index
    screenover[type] = ScreenOverlay();
    if (type >= OVER_FIRSTFREE)
        over_free_ids.push(type);

    reset_drawobj_for_overlay(type);
}

void remove_all_overlays()
{
    for (auto &over : screenover)
        remove_screen_overlay(over.type);
}

ScreenOverlay *get_overlay(int type)
{
    return (type >= 0 && static_cast<uint32_t>(type) < screenover.size() &&
        screenover[type].type >= 0) ? &screenover[type] : nullptr;
}

static size_t add_screen_overlay_impl(bool roomlayer, int x, int y, int type, int sprnum, Bitmap *piccy,
    int pic_offx, int pic_offy)
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

    ScreenOverlay over;
    if (piccy)
    {
        over.SetImage(std::unique_ptr<Bitmap>(piccy), pic_offx, pic_offy);
    }
    else
    {
        over.SetSpriteNum(sprnum, pic_offx, pic_offy);
    }
    over.x=x;
    over.y=y;
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
        {
            create_scriptoverlay(over, true);
            play.speech_text_schandle = over.associatedOverlayHandle;
        }
    }
    else if (type == OVER_PICTURE)
    {
        create_scriptoverlay(over, true);
        play.speech_face_schandle = over.associatedOverlayHandle;
    }
    over.MarkChanged();
    screenover[type] = std::move(over);
    return type;
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, int sprnum)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, sprnum, nullptr, 0, 0);
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, Bitmap *piccy)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, -1, piccy, 0, 0);
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, Common::Bitmap *piccy, int pic_offx, int pic_offy)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, -1, piccy, pic_offx, pic_offy);
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
        const Point screenpt = view->RoomToScreen(
            game.chars[charid].x,
            charextra[charid].GetEffectiveY(&game.chars[charid]) - height).first;
        const Size pic_size = over.GetGraphicSize();
        int tdxp = std::max(0, screenpt.X - pic_size.Width / 2);
        int tdyp = screenpt.Y - 5;
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
        int tdxp = over.x + over.offsetX;
        int tdyp = over.y + over.offsetY;
        if (over.IsRoomRelative())
            return play.RoomToScreen(tdxp, tdyp);
        return Point(tdxp, tdyp);
    }
}

Point update_overlay_graphicspace(ScreenOverlay &over)
{
    Point pos = get_overlay_position(over);
    Bitmap *pic = over.GetImage();
    over._gs = GraphicSpace(pos.X, pos.Y, pic->GetWidth(), pic->GetHeight(),
        pic->GetWidth(), pic->GetHeight(), over.rotation);
    return Point(pos.X, pos.Y);
}

// For software renderer - apply all supported Overlay transforms
Bitmap *recreate_overlay_image(ScreenOverlay &over, Bitmap *&scalebmp, Bitmap *&rotbmp)
{
    Bitmap *pic = over.GetImage();
    Bitmap *use_bmp = pic;
    if (over.rotation != 0.0)
    {
        Size final_sz = RotateSize(use_bmp->GetSize(), over.rotation);
        rotbmp = recycle_bitmap(rotbmp, use_bmp->GetColorDepth(),
            final_sz.Width, final_sz.Height, false);
        const int dst_w = final_sz.Width;
        const int dst_h = final_sz.Height;
        // (+ width%2 fixes one pixel offset problem)
        rotbmp->ClearTransparent();
        rotbmp->RotateBlt(use_bmp, dst_w / 2 + dst_w % 2, dst_h / 2,
            use_bmp->GetWidth() / 2, use_bmp->GetHeight() / 2, over.rotation); // clockwise
        use_bmp = rotbmp;
    }
    if (pic->GetSize() != Size(over.scaleWidth, over.scaleHeight))
    {
        Size final_sz = RotateSize(Size(over.scaleWidth, over.scaleHeight), over.rotation);
        scalebmp = recycle_bitmap(scalebmp, pic->GetColorDepth(), over.scaleWidth, over.scaleHeight);
        scalebmp->StretchBlt(use_bmp, RectWH(scalebmp->GetSize()));
        use_bmp = scalebmp;
    }
    return use_bmp;
}

void restore_overlays()
{
    // Will have to readjust free ids records, as overlays may be restored in any random slots
    while (!over_free_ids.empty()) { over_free_ids.pop(); }
    for (size_t i = 0; i < screenover.size(); ++i)
    {
        auto &over = screenover[i];
        if (over.type >= 0)
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

RuntimeScriptValue Sc_Overlay_CreateGraphical(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptOverlay, Overlay_CreateGraphical);
}

RuntimeScriptValue Sc_Overlay_CreateGraphical5(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL2(ScriptOverlay, Overlay_CreateGraphical5);
}

RuntimeScriptValue Sc_Overlay_CreateRoomGraphical(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptOverlay, Overlay_CreateRoomGraphical);
}

RuntimeScriptValue Sc_Overlay_CreateRoomGraphical5(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL2(ScriptOverlay, Overlay_CreateRoomGraphical5);
}

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

RuntimeScriptValue Sc_Overlay_GetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetBlendMode);
}

RuntimeScriptValue Sc_Overlay_SetBlendMode(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetBlendMode);
}

RuntimeScriptValue Sc_Overlay_GetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_FLOAT(ScriptOverlay, Overlay_GetRotation);
}

RuntimeScriptValue Sc_Overlay_SetRotation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PFLOAT(ScriptOverlay, Overlay_SetRotation);
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
    // WARNING: Overlay.CreateGraphical and CreateRoomGraphical have 1 param REMOVED since ags4
    ScFnRegister overlay_api[] = {
        { "Overlay::CreateGraphical^4",   API_FN_PAIR(Overlay_CreateGraphical) },
        { "Overlay::CreateGraphical^5",   API_FN_PAIR(Overlay_CreateGraphical5) },
        { "Overlay::CreateTextual^106",   Sc_Overlay_CreateTextual, ScPl_Overlay_CreateTextual },
        { "Overlay::CreateRoomGraphical^4", API_FN_PAIR(Overlay_CreateRoomGraphical) },
        { "Overlay::CreateRoomGraphical^5", API_FN_PAIR(Overlay_CreateRoomGraphical5) },
        { "Overlay::CreateRoomTextual^106", Sc_Overlay_CreateRoomTextual, ScPl_Overlay_CreateRoomTextual },
        { "Overlay::SetText^104",         Sc_Overlay_SetText, ScPl_Overlay_SetText },
        { "Overlay::Remove^0",            API_FN_PAIR(Overlay_Remove) },
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
        { "Overlay::get_ZOrder",          API_FN_PAIR(Overlay_GetZOrder) },
        { "Overlay::set_ZOrder",          API_FN_PAIR(Overlay_SetZOrder) },

        { "Overlay::get_BlendMode",       API_FN_PAIR(Overlay_GetBlendMode) },
        { "Overlay::set_BlendMode",       API_FN_PAIR(Overlay_SetBlendMode) },
        { "Overlay::get_Rotation",        API_FN_PAIR(Overlay_GetRotation) },
        { "Overlay::set_Rotation",        API_FN_PAIR(Overlay_SetRotation) },
    };

    ccAddExternalFunctions(overlay_api);
}
