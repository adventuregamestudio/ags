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
#include "ac/common.h"
#include "ac/view.h"
#include "ac/characterextras.h"
#include "ac/draw.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_overlay.h"
#include "ac/global_translation.h"
#include "ac/runtime_defines.h"
#include "ac/screenoverlay.h"
#include "ac/string.h"
#include "gfx/graphicsdriver.h"
#include "gfx/bitmap.h"
#include "script/runtimescriptvalue.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern int displayed_room;
extern int face_talking;
extern ViewStruct*views;
extern CharacterExtras *charextra;
extern IGraphicsDriver *gfxDriver;



ScreenOverlay screenover[MAX_SCREEN_OVERLAYS];
int numscreenover=0;
int is_complete_overlay=0,is_text_overlay=0;
int crovr_id=2;  // whether using SetTextOverlay or CreateTextOvelay

void Overlay_Remove(ScriptOverlay *sco) {
    sco->Remove();
}

void Overlay_SetText(ScriptOverlay *scover, int wii, int fontid, int clr, const char*text) {
    int ovri=find_overlay_of_type(scover->overlayId);
    if (ovri<0)
        quit("!Overlay.SetText: invalid overlay ID specified");
    int xx = divide_down_coordinate(screenover[ovri].x) - scover->borderWidth;
    int yy = divide_down_coordinate(screenover[ovri].y) - scover->borderHeight;

    RemoveOverlay(scover->overlayId);
    crovr_id = scover->overlayId;

    if (CreateTextOverlay(xx,yy,wii,fontid,clr,get_translation(text)) != scover->overlayId)
        quit("SetTextOverlay internal error: inconsistent type ids");
}

int Overlay_GetX(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    int tdxp, tdyp;
    get_overlay_position(ovri, &tdxp, &tdyp);

    return divide_down_coordinate(tdxp);
}

void Overlay_SetX(ScriptOverlay *scover, int newx) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    screenover[ovri].x = multiply_up_coordinate(newx);
}

int Overlay_GetY(ScriptOverlay *scover) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    int tdxp, tdyp;
    get_overlay_position(ovri, &tdxp, &tdyp);

    return divide_down_coordinate(tdyp);
}

void Overlay_SetY(ScriptOverlay *scover, int newy) {
    int ovri = find_overlay_of_type(scover->overlayId);
    if (ovri < 0)
        quit("!invalid overlay ID specified");

    screenover[ovri].y = multiply_up_coordinate(newy);
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

ScriptOverlay* Overlay_CreateGraphical(int x, int y, int slot, int transparent) {
    ScriptOverlay *sco = new ScriptOverlay();
    sco->overlayId = CreateGraphicOverlay(x, y, slot, transparent);
    sco->borderHeight = 0;
    sco->borderWidth = 0;
    sco->isBackgroundSpeech = 0;

    ccRegisterManagedObject(sco, sco);
    return sco;
}

ScriptOverlay* Overlay_CreateTextual(int x, int y, int width, int font, int colour, const char* text) {
    ScriptOverlay *sco = new ScriptOverlay();

    multiply_up_coordinates(&x, &y);
    width = multiply_up_coordinate(width);

    sco->overlayId = CreateTextOverlayCore(x, y, width, font, colour, text, 0);

    int ovri = find_overlay_of_type(sco->overlayId);
    sco->borderWidth = divide_down_coordinate(screenover[ovri].x - x);
    sco->borderHeight = divide_down_coordinate(screenover[ovri].y - y);
    sco->isBackgroundSpeech = 0;

    ccRegisterManagedObject(sco, sco);
    return sco;
}

//=============================================================================

void remove_screen_overlay_index(int cc) {
    int dd;
    delete screenover[cc].pic;
    screenover[cc].pic=NULL;

    if (screenover[cc].bmp != NULL)
        gfxDriver->DestroyDDB(screenover[cc].bmp);
    screenover[cc].bmp = NULL;

    if (screenover[cc].type==OVER_COMPLETE) is_complete_overlay--;
    if (screenover[cc].type==OVER_TEXTMSG) is_text_overlay--;

    // if the script didn't actually use the Overlay* return
    // value, dispose of the pointer
    if (screenover[cc].associatedOverlayHandle)
        ccAttemptDisposeObject(screenover[cc].associatedOverlayHandle);

    numscreenover--;
    for (dd = cc; dd < numscreenover; dd++)
        screenover[dd] = screenover[dd+1];

    // if an overlay before the sierra-style speech one is removed,
    // update the index
    if (face_talking > cc)
        face_talking--;
}

void remove_screen_overlay(int type) {
    int cc;
    for (cc=0;cc<numscreenover;cc++) {
        if (screenover[cc].type==type) ;
        else if (type==-1) ;
        else continue;
        remove_screen_overlay_index(cc);
        cc--;
    }
}

int find_overlay_of_type(int typ) {
    int ww;
    for (ww=0;ww<numscreenover;ww++) {
        if (screenover[ww].type == typ) return ww;
    }
    return -1;
}

int add_screen_overlay(int x,int y,int type,Bitmap *piccy, bool alphaChannel) {
    if (numscreenover>=MAX_SCREEN_OVERLAYS)
        quit("too many screen overlays created");
    if (type==OVER_COMPLETE) is_complete_overlay++;
    if (type==OVER_TEXTMSG) is_text_overlay++;
    if (type==OVER_CUSTOM) {
        int uu;  // find an unused custom ID
        for (uu=OVER_CUSTOM+1;uu<OVER_CUSTOM+100;uu++) {
            if (find_overlay_of_type(uu) == -1) { type=uu; break; }
        }
    }
    screenover[numscreenover].pic=piccy;
    screenover[numscreenover].bmp = gfxDriver->CreateDDBFromBitmap(piccy, alphaChannel);
    screenover[numscreenover].x=x;
    screenover[numscreenover].y=y;
    screenover[numscreenover].type=type;
    screenover[numscreenover].timeout=0;
    screenover[numscreenover].bgSpeechForChar = -1;
    screenover[numscreenover].associatedOverlayHandle = 0;
    screenover[numscreenover].hasAlphaChannel = alphaChannel;
    screenover[numscreenover].positionRelativeToScreen = true;
    numscreenover++;
    return numscreenover-1;
}



void get_overlay_position(int overlayidx, int *x, int *y) {
    int tdxp, tdyp;
    const Rect &ui_view = play.GetUIViewport();
    const Rect &camera = play.GetRoomCamera();

    if (screenover[overlayidx].x == OVR_AUTOPLACE) {
        // auto place on character
        int charid = screenover[overlayidx].y;
        int charpic = views[game.chars[charid].view].loops[game.chars[charid].loop].frames[0].pic;

        tdyp = multiply_up_coordinate(game.chars[charid].get_effective_y()) - camera.Top - 5;
        if (charextra[charid].height<1)
            tdyp -= game.SpriteInfos[charpic].Height;
        else
            tdyp -= charextra[charid].height;

        tdyp -= screenover[overlayidx].pic->GetHeight();
        if (tdyp < 5) tdyp=5;
        tdxp = (multiply_up_coordinate(game.chars[charid].x) - screenover[overlayidx].pic->GetWidth()/2) - camera.Left;
        if (tdxp < 0) tdxp=0;

        if ((tdxp + screenover[overlayidx].pic->GetWidth()) >= ui_view.GetWidth())
            tdxp = (ui_view.GetWidth() - screenover[overlayidx].pic->GetWidth()) - 1;
        if (game.chars[charid].room != displayed_room) {
            tdxp = ui_view.GetWidth()/2 - screenover[overlayidx].pic->GetWidth()/2;
            tdyp = ui_view.GetHeight()/2 - screenover[overlayidx].pic->GetHeight()/2;
        }
    }
    else {
        tdxp = screenover[overlayidx].x;
        tdyp = screenover[overlayidx].y;

        if (!screenover[overlayidx].positionRelativeToScreen)
        {
            tdxp -= camera.Left;
            tdyp -= camera.Top;
        }
    }
    *x = tdxp;
    *y = tdyp;
}

void recreate_overlay_ddbs()
{
    for (int i = 0; i < numscreenover; ++i)
    {
        if (screenover[i].bmp)
            gfxDriver->DestroyDDB(screenover[i].bmp);
        if (screenover[i].pic)
            screenover[i].bmp = gfxDriver->CreateDDBFromBitmap(screenover[i].pic, false);
        else
            screenover[i].bmp = NULL;
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
    API_SCALL_OBJAUTO_PINT4(ScriptOverlay, Overlay_CreateGraphical);
}

// ScriptOverlay* (int x, int y, int width, int font, int colour, const char* text, ...)
RuntimeScriptValue Sc_Overlay_CreateTextual(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(Overlay_CreateTextual, 6);
    ScriptOverlay *overlay = Overlay_CreateTextual(params[0].IValue, params[1].IValue, params[2].IValue,
                                                   params[3].IValue, params[4].IValue, scsf_buffer);
    return RuntimeScriptValue().SetDynamicObject(overlay, overlay);
}

// void (ScriptOverlay *scover, int wii, int fontid, int clr, char*texx, ...)
RuntimeScriptValue Sc_Overlay_SetText(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_SCRIPT_SPRINTF(Overlay_SetText, 4);
    Overlay_SetText((ScriptOverlay*)self, params[0].IValue, params[1].IValue, params[2].IValue, scsf_buffer);
    return RuntimeScriptValue();
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
    ccAddExternalStaticFunction("Overlay::CreateTextual^106",   Sc_Overlay_CreateTextual);
    ccAddExternalObjectFunction("Overlay::SetText^104",         Sc_Overlay_SetText);
    ccAddExternalObjectFunction("Overlay::Remove^0",            Sc_Overlay_Remove);
    ccAddExternalObjectFunction("Overlay::get_Valid",           Sc_Overlay_GetValid);
    ccAddExternalObjectFunction("Overlay::get_X",               Sc_Overlay_GetX);
    ccAddExternalObjectFunction("Overlay::set_X",               Sc_Overlay_SetX);
    ccAddExternalObjectFunction("Overlay::get_Y",               Sc_Overlay_GetY);
    ccAddExternalObjectFunction("Overlay::set_Y",               Sc_Overlay_SetY);

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
