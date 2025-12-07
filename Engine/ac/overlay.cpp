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
#include "ac/game.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_game.h"
#include "ac/global_translation.h"
#include "ac/object.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "ac/spritecache.h"
#include "ac/string.h"
#include "ac/dynobj/dynobj_manager.h"
#include "ac/dynobj/scriptshader.h"
#include "debug/debug_log.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "main/game_run.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern SpriteCache spriteset;
extern int displayed_room;
extern int face_talking;
extern std::vector<ViewStruct> views;
extern IGraphicsDriver *gfxDriver;

// We store overlays in a container where items are never erased:
// this makes things optimized for very fast mass object adding/removal.
// TODO: consider some kind of a "object pool" template,
// which handles this kind of storage; share with ManagedPool's handles?
std::vector<ScreenOverlay> screenover;
std::queue<int32_t> over_free_ids;
std::vector<AnimatedOverlay> animovers;
std::vector<size_t> over_to_anim; // overlay to animated overlay lookup
std::queue<size_t> animover_free_ids;

// Gets an actual ScreenOverlay object from its ScriptOverlay reference,
// validate object, throw an error on failure
static ScreenOverlay *GetOverlayValidate(const char *apiname, ScriptOverlay *scover)
{
    auto *over = get_overlay(scover->GetOverlayID());
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
    DisplayTextPosition text_pos = over.IsAutoPosition() ? kDisplayTextPos_Overchar : kDisplayTextPos_Normal;
    // allow DisplaySpeechBackground to be shrunk
    DisplayTextShrink allow_shrink = over.IsAutoPosition() ? kDisplayTextShrink_Left : kDisplayTextShrink_None;

    // from Overlay_CreateTextCore
    if (width < 8) width = play.GetUIViewport().GetWidth() / 2;
    if (text_color == 0) text_color = GUI::GetStandardColor(16);

    const char *draw_text = get_translation(text);
    // Skip a voice-over token, if present
    draw_text = skip_voiceover_token(draw_text);

    // Recreate overlay image
    int dummy_x = x, dummy_y = y, adj_x = x, adj_y = y;
    std::unique_ptr<Bitmap> image = create_textual_image(draw_text,
        DisplayTextLooks(kDisplayTextStyle_TextWindow, text_pos, allow_shrink),
        text_color, dummy_x, dummy_y, adj_x, adj_y, width, fontid, nullptr);

    // Update overlay properties
    over.SetImage(std::move(image), adj_x - dummy_x, adj_y - dummy_y);
}

int Overlay_GetX(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.X", scover);
    if (over->IsAutoPosition())
        autoposition_overlay(*over); // in case they moved character in script, etc
    return over->GetX();
}

void Overlay_SetX(ScriptOverlay *scover, int newx)
{
    auto *over = GetOverlayValidate("Overlay.X", scover);
    over->SetFixedPosition(newx, over->GetY());
}

int Overlay_GetY(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Y", scover);
    if (over->IsAutoPosition())
        autoposition_overlay(*over); // in case they moved character in script, etc
    return over->GetY();
}

void Overlay_SetY(ScriptOverlay *scover, int newy)
{
    auto *over = GetOverlayValidate("Overlay.Y", scover);
    over->SetFixedPosition(over->GetX(), newy);
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
    return over->GetScaledWidth();
}

int Overlay_GetHeight(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Height", scover);
    return over->GetScaledHeight();
}

int Overlay_GetGraphicWidth(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.GraphicWidth", scover);
    return over->GetGraphicSize().Width;
}

int Overlay_GetGraphicHeight(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.GraphicHeight", scover);
    return over->GetGraphicSize().Height;
}

void Overlay_SetWidth(ScriptOverlay *scover, int width)
{
    auto *over = GetOverlayValidate("Overlay.Width", scover);
    if (width > 0)
        over->SetScaledSize(width, over->GetScaledHeight());
    else
        debug_script_warn("Overlay.SetWidth: invalid width %d", width);
}

void Overlay_SetHeight(ScriptOverlay *scover, int height)
{
    auto *over = GetOverlayValidate("Overlay.Height", scover);
    if (height > 0)
        over->SetScaledSize(over->GetScaledWidth(), height);
    else
        debug_script_warn("Overlay.SetWidth: invalid height %d", height);
}

int Overlay_GetFlip(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Flip", scover);
    return over->GetFlip();
}

void Overlay_SetFlip(ScriptOverlay *scover, int flip)
{
    auto *over = GetOverlayValidate("Overlay.Flip", scover);
    over->SetFlip(ValidateFlip("Overlay.Flip", flip));
}

int Overlay_GetValid(ScriptOverlay *scover)
{
    return get_overlay(scover->GetOverlayID()) != nullptr;
}

ScreenOverlay *Overlay_CreateGraphicCore(bool room_layer, int x, int y, int slot, bool clone)
{
    if (!spriteset.DoesSpriteExist(slot))
    {
        debug_script_warn("Overlay.CreateGraphical: sprite %d is invalid", slot);
        return nullptr;
    }

    size_t overid;
    // We clone only dynamic sprites, because it makes no sense to clone normal ones
    if (clone && ((game.SpriteInfos[slot].Flags & SPF_DYNAMICALLOC) != 0))
    {
        std::unique_ptr<Bitmap> screeno(new Bitmap(game.SpriteInfos[slot].Width, game.SpriteInfos[slot].Height, game.GetColorDepth()));
        screeno->Blit(spriteset[slot]);
        overid = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, std::move(screeno));
    }
    else
    {
        overid = add_screen_overlay(room_layer, x, y, OVER_CUSTOM, slot);
    }
    return overid < SIZE_MAX ? &screenover[overid] : nullptr;
}

ScreenOverlay *Overlay_CreateTextCore(bool room_layer, int x, int y, int width, int font, int text_color,
    const char *text, int over_type, DisplayTextStyle style, DisplayTextShrink allow_shrink, int speech_for_char)
{
    // NOTE: this was not documented, but apparently passing x < 0 or y < 0
    // to Overlay.CreateTextual actually made it centered on screen
    DisplayTextPosition text_pos = get_textpos_from_scriptcoords(x, y, false);
    if (width < 8) width = play.GetUIViewport().GetWidth() / 2;
    if (text_color == 0) text_color = GUI::GetStandardColor(16);
    // Skip a voice-over token, if present
    const char *draw_text = skip_voiceover_token(text);
    return display_main(x, y, width, draw_text, nullptr, kDisplayText_NormalOverlay, over_type,
        DisplayTextLooks(style, text_pos, allow_shrink),
        font, text_color, speech_for_char, room_layer);
}

ScriptOverlay* Overlay_CreateGraphicalImpl(bool room_layer, int x, int y, int slot, bool clone)
{
    auto *over = Overlay_CreateGraphicCore(room_layer, x, y, slot, clone);
    return over ? over->CreateScriptObject() : nullptr;
}

ScriptOverlay* Overlay_CreateTextualImpl(bool room_layer, int x, int y, int width, int font, int colour, const char* text)
{
    auto *over = Overlay_CreateTextCore(room_layer, x, y, width, font, colour, text, OVER_CUSTOM, kDisplayTextStyle_TextWindow, kDisplayTextShrink_None);
    return over ? over->CreateScriptObject() : nullptr;
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

int Overlay_GetBlendMode(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.BlendMode", scover);
    return over->GetBlendMode();
}

void Overlay_SetBlendMode(ScriptOverlay *scover, int blend_mode)
{
    auto *over = GetOverlayValidate("Overlay.BlendMode", scover);
    over->SetBlendMode(ValidateBlendMode("Overlay.BlendMode", "", blend_mode));
}

ScriptShaderInstance *Overlay_GetShader(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.Shader", scover);
    return static_cast<ScriptShaderInstance*>(ccGetObjectAddressFromHandle(over->GetShaderHandle()));
}

void Overlay_SetShader(ScriptOverlay *scover, ScriptShaderInstance *shader_inst)
{
    auto *over = GetOverlayValidate("Overlay.Shader", scover);
    over->SetShader(shader_inst ? shader_inst->GetID() : ScriptShaderInstance::NullInstanceID,
                    ccReplaceObjectHandle(over->GetShaderHandle(), shader_inst));
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

float Overlay_GetRotation(ScriptOverlay *scover) {
    auto *over = GetOverlayValidate("Overlay.Rotation", scover);
    return over->GetRotation();
}

void Overlay_SetRotation(ScriptOverlay *scover, float degrees) {
    auto *over = GetOverlayValidate("Overlay.Rotation", scover);
    over->SetRotation(Math::ClampAngle360(degrees));
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

void Overlay_SetPosition(ScriptOverlay *scover, int x, int y, int width, int height)
{
    auto *over = GetOverlayValidate("Overlay.SetPosition", scover);
    over->SetFixedPosition(x, y);
    // width and height are optional here
    if (width > 0 || height > 0)
    {
        width = width > 0 ? width : over->GetScaledWidth();
        height = height > 0 ? height : over->GetScaledHeight();
        over->SetScaledSize(width, height);
    }
}

void Overlay_SetSize(ScriptOverlay *scover, int width, int height)
{
    auto *over = GetOverlayValidate("Overlay.SetSize", scover);
    if (width > 0 && height > 0)
        over->SetScaledSize(width, height);
    else
        debug_script_warn("Overlay.SetSize: invalid dimensions: %d x %d", width, height);
}

void Overlay_Tint(ScriptOverlay *scover, int red, int green, int blue, int opacity, int luminance)
{
    auto *over = GetOverlayValidate("Overlay.Tint", scover);

    if ((red < 0) || (green < 0) || (blue < 0) ||
        (red > 255) || (green > 255) || (blue > 255) ||
        (opacity < 0) || (opacity > 100) ||
        (luminance < 0) || (luminance > 100))
    {
        debug_script_warn("Overlay.Tint: invalid parameter(s). R,G,B must be 0-255 (passed: %d,%d,%d), opacity & luminance 0-100 (passed: %d,%d)",
            red, green, blue, opacity, luminance);
        return;
    }

    over->SetTint(red, green, blue, opacity, GfxDef::Value100ToValue250(luminance));
}

void Overlay_SetLightLevel(ScriptOverlay *scover, int light_level)
{
    auto *over = GetOverlayValidate("Overlay.SetLightLevel", scover);

    over->SetLightLevel(Math::Clamp(light_level, -100, 100));
}

void Overlay_RemoveTint(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.RemoveTint", scover);

    over->RemoveTint();
}

bool Overlay_GetHasLight(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.HasLight", scover);
    return over->HasLightLevel();
}

bool Overlay_GetHasTint(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.HasTint", scover);
    return over->HasTint();
}

int Overlay_GetLightLevel(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.LightLevel", scover);
    return over->HasLightLevel() ? over->GetTintLight() : 0;
}

int Overlay_GetTintRed(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.TintRed", scover);
    return over->HasTint() ? over->GetTintR() : 0;
}

int Overlay_GetTintGreen(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.TintGreen", scover);
    return over->HasTint() ? over->GetTintG() : 0;
}

int Overlay_GetTintBlue(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.TintBlue", scover);
    return over->HasTint() ? over->GetTintB() : 0;
}

int Overlay_GetTintSaturation(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.TintSaturation", scover);
    return over->HasTint() ? over->GetTintLevel() : 0;
}

int Overlay_GetTintLuminance(ScriptOverlay *scover)
{
    auto *over = GetOverlayValidate("Overlay.TintLuminance", scover);
    return over->HasTint() ? GfxDef::Value250ToValue100(over->GetTintLight()) : 0;
}

//=============================================================================

ScriptAnimatedOverlay *create_scriptanimoverlay(ScreenOverlay &over)
{
    ScriptAnimatedOverlay *scover = new ScriptAnimatedOverlay(over.GetID());
    over.AssignScriptObject(scover);
    return scover;
}

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

static size_t add_screen_overlay_impl(bool roomlayer, int x, int y, int type, int sprnum,
    std::unique_ptr<Bitmap> piccy, int pic_offx, int pic_offy)
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
        over.SetImage(std::move(piccy), pic_offx, pic_offy);
    }
    else
    {
        over.SetSpriteNum(sprnum, pic_offx, pic_offy);
    }
    over.SetFixedPosition(x, y);
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
    return add_screen_overlay_impl(roomlayer, x, y, type, sprnum, nullptr, 0, 0);
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, std::unique_ptr<Bitmap> piccy)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, -1, std::move(piccy), 0, 0);
}

size_t add_screen_overlay(bool roomlayer, int x, int y, int type, std::unique_ptr<Bitmap> piccy, int pic_offx, int pic_offy)
{
    return add_screen_overlay_impl(roomlayer, x, y, type, -1, std::move(piccy), pic_offx, pic_offy);
}

Point get_overlay_display_pos(const ScreenOverlay &over)
{
    // Note: the internal offset is only needed when x,y coordinates are specified
    // and only in the case where the overlay is using a GUI. See issue #1098
    return Point(over.GetDrawX(), over.GetDrawY());
}

// Calculates overlay position above linked character
// TODO: this code assumes that overlay is in the screen layer;
// perhaps add a branch for room layer as well?
void autoposition_overlay(ScreenOverlay &over)
{
    assert(over.IsAutoPosition());
    if (!over.IsAutoPosition() || (over.GetCharacterRef() < 0))
        return;

    const Rect &ui_view = play.GetUIViewport();
    const int charid = over.GetCharacterRef();

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

    over.SetAutoPosition(charid, tdxp, tdyp);
}

void restore_overlays()
{
    // Will have to readjust free ids records, as overlays may be restored in any random slots
    over_free_ids = std::queue<int32_t>();
    for (size_t i = 0; i < screenover.size(); ++i)
    {
        auto &over = screenover[i];
        if (over.GetID() >= 0)
        {
            if (over.IsAutoPosition())
                autoposition_overlay(over);
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

void CreateAnimatedOverlay(int over_id, bool pause_with_game)
{
    AddAnimatedOverlay(AnimatedOverlay(over_id, pause_with_game));
}

static void UpdateOverlayState(const AnimatedOverlay &aover)
{
    auto &over = screenover[aover.GetOverID()];
    const ViewFrame *vf = aover.GetViewFrame();
    if (vf)
    {
        over.SetSpriteNum(vf->pic, vf->xoffs, vf->yoffs);
        over.SetFlip(GfxDef::GetFlipFromFlags(vf->flags));
    }
}

void BeginAnimateOverlay(int over_id, int view, int loop, int frame, const ViewAnimateParams &params)
{
    auto *aover = GetOverlayAnimation(over_id);
    if (!aover)
        return;

    aover->Begin(view, loop, frame, params);
    UpdateOverlayState(*aover);
}

const std::vector<AnimatedOverlay> &GetAnimateOverlays()
{
    return animovers;
}

size_t AddAnimatedOverlay(AnimatedOverlay &&aover)
{
    const int over_id = aover.GetOverID();
    if (over_id < 0)
        return SIZE_MAX;

    if (static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
    {
        animovers[over_to_anim[over_id]] = std::move(aover);
        return over_to_anim[over_id];
    }
    else
    {
        // Find free id
        size_t aover_id;
        if (animover_free_ids.size() > 0)
        {
            aover_id = animover_free_ids.front();
            animover_free_ids.pop();
        }
        else
        {
            aover_id = animovers.size();
            animovers.resize(aover_id + 1);
        }

        animovers[aover_id] = std::move(aover);
        if (static_cast<uint32_t>(over_id) >= over_to_anim.size())
            over_to_anim.resize(over_id + 1, SIZE_MAX);
        over_to_anim[over_id] = aover_id;
        return aover_id;
    }
}

const std::vector<AnimatedOverlay> &GetAnimatedOverlays()
{
    return animovers;
}

bool IsOverlayAnimating(int over_id)
{
    if (over_id >= 0 && static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
        return animovers[over_to_anim[over_id]].IsAnimating();
    else
        return false;
}

AnimatedOverlay *GetOverlayAnimation(int over_id)
{
    if (over_id >= 0 && static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
        return &animovers[over_to_anim[over_id]];
    return nullptr;
}

static void StopOverlayAnimImpl(size_t aover_id)
{
    animovers[aover_id].Reset();
}

void UpdateOverlayAnimations()
{
    for (size_t i = 0; i < animovers.size(); ++i)
    {
        auto &aover = animovers[i];
        if (!aover.IsAnimating())
            continue;
        if (aover.GetPauseWithGame() && IsGamePaused())
            continue;

        if (aover.UpdateOnce())
        {
            UpdateOverlayState(aover);
        }
        else
        {
            StopOverlayAnimImpl(i);
        }
    }
}

void StopOverlayAnimation(int over_id)
{
    if (over_id >= 0 && static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
    {
        StopOverlayAnimImpl(over_to_anim[over_id]);
    }
}

void RemoveAnimatedOverlay(int over_id)
{
    if (over_id >= 0 && static_cast<uint32_t>(over_id) < over_to_anim.size() && over_to_anim[over_id] != SIZE_MAX)
    {
        size_t aover_id = over_to_anim[over_id];
        animovers[aover_id] = {};
        animover_free_ids.push(aover_id);
        over_to_anim[over_id] = SIZE_MAX;
    }
}

void RemoveAllAnimatedOverlays()
{
    animovers.clear();
    over_to_anim.clear();
    animover_free_ids = std::queue<size_t>();
}

//=============================================================================

ScriptAnimatedOverlay *AnimatedOverlay_CreateAnimatedImpl(bool room_layer, int x, int y, int slot, bool pause_with_game)
{
    auto *over = Overlay_CreateGraphicCore(room_layer, x, y, slot, false);
    if (!over)
        return nullptr;
    CreateAnimatedOverlay(over->GetID(), pause_with_game);
    return create_scriptanimoverlay(*over);
}

ScriptAnimatedOverlay *AnimatedOverlay_CreateAnimated(int x, int y, int slot, bool pause_with_game)
{
    return AnimatedOverlay_CreateAnimatedImpl(false, x, y, slot, pause_with_game);
}

ScriptAnimatedOverlay *AnimatedOverlay_CreateRoomAnimated(int x, int y, int slot, bool pause_with_game)
{
    return AnimatedOverlay_CreateAnimatedImpl(true, x, y, slot, pause_with_game);
}

void AnimatedOverlay_Animate(ScriptAnimatedOverlay *scover, int view, int loop, int speed, int repeat,
                             int blocking, int direction, int sframe, int volume)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.Animate", scover);

    view--; // convert to internal 0-based view ID
    ValidateViewAnimVLF("AnimatedOverlay.Animate", "", view, loop, sframe);
    ValidateViewAnimParams("AnimatedOverlay.Animate", "", blocking, repeat, direction);

    volume = Math::Clamp(volume, 0, 100);

    BeginAnimateOverlay(over->GetID(), view, loop, sframe,
                        ViewAnimateParams(static_cast<AnimFlowStyle>(repeat), static_cast<AnimFlowDirection>(direction), speed, volume));

    // Blocking animate
    if (blocking)
        GameLoopUntilOverlayAnimEnd(over->GetID());
}

bool AnimatedOverlay_GetAnimating(ScriptAnimatedOverlay *scover)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.Animate", scover);
    return IsOverlayAnimating(over->GetID());
}

int AnimatedOverlay_GetFrame(ScriptAnimatedOverlay *scover)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.Frame", scover);
    const auto *aover = GetOverlayAnimation(over->GetID());
    return aover ? aover->GetFrame() : -1;
}

int AnimatedOverlay_GetLoop(ScriptAnimatedOverlay *scover)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.Loop", scover);
    const auto *aover = GetOverlayAnimation(over->GetID());
    return aover ? aover->GetLoop() : -1;
}

bool AnimatedOverlay_GetPauseWithGame(ScriptAnimatedOverlay *scover)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.PauseWithGame", scover);
    const auto *aover = GetOverlayAnimation(over->GetID());
    return aover ? aover->GetPauseWithGame() : false;
}

int AnimatedOverlay_GetView(ScriptAnimatedOverlay *scover)
{
    auto *over = GetOverlayValidate("AnimatedOverlay.View", scover);
    const auto *aover = GetOverlayAnimation(over->GetID());
    return aover ? aover->GetView() + 1 : 0; // convert view index from internal to script
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

RuntimeScriptValue Sc_Overlay_GetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_OBJAUTO(ScriptOverlay, ScriptShaderInstance, Overlay_GetShader);
}

RuntimeScriptValue Sc_Overlay_SetShader(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_POBJ(ScriptOverlay, Overlay_SetShader, ScriptShaderInstance);
}

RuntimeScriptValue Sc_Overlay_GetFlip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetFlip);
}

RuntimeScriptValue Sc_Overlay_SetFlip(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetFlip);
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

RuntimeScriptValue Sc_Overlay_Tint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT5(ScriptOverlay, Overlay_Tint);
}

RuntimeScriptValue Sc_Overlay_SetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT(ScriptOverlay, Overlay_SetLightLevel);
}

RuntimeScriptValue Sc_Overlay_RemoveTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID(ScriptOverlay, Overlay_RemoveTint);
}

RuntimeScriptValue Sc_Overlay_GetHasLight(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptOverlay, Overlay_GetHasLight);
}

RuntimeScriptValue Sc_Overlay_GetHasTint(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptOverlay, Overlay_GetHasTint);
}

RuntimeScriptValue Sc_Overlay_GetLightLevel(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetLightLevel);
}

RuntimeScriptValue Sc_Overlay_GetTintBlue(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintBlue);
}

RuntimeScriptValue Sc_Overlay_GetTintGreen(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintGreen);
}

RuntimeScriptValue Sc_Overlay_GetTintRed(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintRed);
}

RuntimeScriptValue Sc_Overlay_GetTintSaturation(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintSaturation);
}

RuntimeScriptValue Sc_Overlay_GetTintLuminance(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptOverlay, Overlay_GetTintLuminance);
}

RuntimeScriptValue Sc_AnimatedOverlay_CreateAnimated(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptAnimatedOverlay, AnimatedOverlay_CreateAnimated);
}

RuntimeScriptValue Sc_AnimatedOverlay_CreateRoomAnimated(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT3_PBOOL(ScriptAnimatedOverlay, AnimatedOverlay_CreateRoomAnimated);
}

RuntimeScriptValue Sc_AnimatedOverlay_Animate(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_VOID_PINT8(ScriptAnimatedOverlay, AnimatedOverlay_Animate);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetAnimating(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_BOOL(ScriptAnimatedOverlay, AnimatedOverlay_GetAnimating);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetView(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAnimatedOverlay, AnimatedOverlay_GetView);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetLoop(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAnimatedOverlay, AnimatedOverlay_GetLoop);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetFrame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAnimatedOverlay, AnimatedOverlay_GetFrame);
}

RuntimeScriptValue Sc_AnimatedOverlay_GetPauseWithGame(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptAnimatedOverlay, AnimatedOverlay_GetPauseWithGame);
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
        { "Overlay::SetPosition^4",       API_FN_PAIR(Overlay_SetPosition) },
        { "Overlay::SetSize^2",           API_FN_PAIR(Overlay_SetSize) },
        { "Overlay::Tint^5",              API_FN_PAIR(Overlay_Tint) },
        { "Overlay::SetLightLevel^1",     API_FN_PAIR(Overlay_SetLightLevel) },
        { "Overlay::RemoveTint^0",        API_FN_PAIR(Overlay_RemoveTint) },
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
        { "Overlay::get_HasLightLevel",   API_FN_PAIR(Overlay_GetHasLight) },
        { "Overlay::get_HasTint",         API_FN_PAIR(Overlay_GetHasTint) },
        { "Overlay::get_LightLevel",      API_FN_PAIR(Overlay_GetLightLevel) },
        { "Overlay::get_TintBlue",        API_FN_PAIR(Overlay_GetTintBlue) },
        { "Overlay::get_TintGreen",       API_FN_PAIR(Overlay_GetTintGreen) },
        { "Overlay::get_TintRed",         API_FN_PAIR(Overlay_GetTintRed) },
        { "Overlay::get_TintSaturation",  API_FN_PAIR(Overlay_GetTintSaturation) },
        { "Overlay::get_TintLuminance",   API_FN_PAIR(Overlay_GetTintLuminance) },

        { "Overlay::get_BlendMode",       API_FN_PAIR(Overlay_GetBlendMode) },
        { "Overlay::set_BlendMode",       API_FN_PAIR(Overlay_SetBlendMode) },
        { "Overlay::get_Rotation",        API_FN_PAIR(Overlay_GetRotation) },
        { "Overlay::set_Rotation",        API_FN_PAIR(Overlay_SetRotation) },
        { "Overlay::get_Flip",            API_FN_PAIR(Overlay_GetFlip) },
        { "Overlay::set_Flip",            API_FN_PAIR(Overlay_SetFlip) },

        { "Overlay::get_Shader",          API_FN_PAIR(Overlay_GetShader) },
        { "Overlay::set_Shader",          API_FN_PAIR(Overlay_SetShader) },
    };

    ccAddExternalFunctions(overlay_api);

    ScFnRegister animoverlay_api[] = {
        { "AnimatedOverlay::CreateAnimated",        API_FN_PAIR(AnimatedOverlay_CreateAnimated) },
        { "AnimatedOverlay::CreateRoomAnimated",    API_FN_PAIR(AnimatedOverlay_CreateRoomAnimated) },
        { "AnimatedOverlay::Animate",               API_FN_PAIR(AnimatedOverlay_Animate) },
        { "AnimatedOverlay::get_Animating",         API_FN_PAIR(AnimatedOverlay_GetAnimating) },
        { "AnimatedOverlay::get_Frame",             API_FN_PAIR(AnimatedOverlay_GetFrame) },
        { "AnimatedOverlay::get_Loop",              API_FN_PAIR(AnimatedOverlay_GetLoop) },
        { "AnimatedOverlay::get_PauseWithGame",     API_FN_PAIR(AnimatedOverlay_GetPauseWithGame) },
        { "AnimatedOverlay::get_View",              API_FN_PAIR(AnimatedOverlay_GetView) },
    };

    ccAddExternalFunctions(animoverlay_api);
}
