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
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

#include "ac/display.h"
#include "ac/dynamicsprite.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/global_audio.h"
#include "ac/global_button.h"
#include "ac/global_character.h"
#include "ac/global_datetime.h"
#include "ac/global_debug.h"
#include "ac/global_dialog.h"
#include "ac/global_display.h"
#include "ac/global_dynamicsprite.h"
#include "ac/global_file.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_hotspot.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_label.h"
#include "ac/global_listbox.h"
#include "ac/global_mouse.h"
#include "ac/global_object.h"
#include "ac/global_palette.h"
#include "ac/global_region.h"
#include "ac/global_room.h"
#include "ac/global_slider.h"
#include "ac/global_screen.h"
#include "ac/global_textbox.h"
#include "ac/global_timer.h"
#include "ac/global_translation.h"
#include "ac/global_video.h"
#include "ac/global_walkablearea.h"
#include "ac/global_walkbehind.h"
#include "ac/math.h"
#include "ac/mouse.h"
#include "ac/parser.h"
#include "ac/string.h"
#include "ac/room.h"
#include "media/video/video.h"
#include "util/string_compat.h"
#include "media/audio/audio_system.h"

#include "ac/dynobj/scriptstring.h"
extern ScriptString myScriptStringImpl;

// void (char*texx, ...)
RuntimeScriptValue Sc_sc_AbortGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(_sc_AbortGame, 1);
    _sc_AbortGame(scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

// void (int guin, int objn, int view, int loop, int speed, int repeat)
RuntimeScriptValue Sc_AnimateButton(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT6(AnimateButton);
}

// int (int thing1, int thing2)
RuntimeScriptValue Sc_AreThingsOverlapping(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(AreThingsOverlapping);
}

// void  (int value)
RuntimeScriptValue Sc_CallRoomScript(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(CallRoomScript);
}

// void  (int ifn)
RuntimeScriptValue Sc_CentreGUI(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(CentreGUI);
}

extern RuntimeScriptValue Sc_ChangeCursorGraphic(const RuntimeScriptValue *params, int32_t param_count);

extern RuntimeScriptValue Sc_ChangeCursorHotspot(const RuntimeScriptValue *params, int32_t param_count);

// void ()
RuntimeScriptValue Sc_ClaimEvent(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(ClaimEvent);
}

// void (int strt,int eend)
RuntimeScriptValue Sc_CyclePalette(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(CyclePalette);
}

// void (int cmdd,int dataa)
RuntimeScriptValue Sc_script_debug(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(script_debug);
}

// void  (int slnum)
RuntimeScriptValue Sc_DeleteSaveSlot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DeleteSaveSlot);
}

// void  (int gotSlot)
RuntimeScriptValue Sc_free_dynamic_sprite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(free_dynamic_sprite);
}

extern RuntimeScriptValue Sc_disable_cursor_mode(const RuntimeScriptValue *params, int32_t param_count);

// void (int alsoEffects)
RuntimeScriptValue Sc_DisableGroundLevelAreas(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DisableGroundLevelAreas);
}

// void (int hsnum)
RuntimeScriptValue Sc_DisableHotspot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DisableHotspot);
}

// void ()
RuntimeScriptValue Sc_DisableInterface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(DisableInterface);
}

// void (int hsnum)
RuntimeScriptValue Sc_DisableRegion(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DisableRegion);
}

// void (char*texx, ...)
RuntimeScriptValue Sc_Display(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(Display, 1);
    DisplaySimple(scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

// void (int xxp,int yyp,int widd,char*texx, ...)
RuntimeScriptValue Sc_DisplayAt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(DisplayAt, 4);
    DisplayAt(params[0].IValue, params[1].IValue, params[2].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

// void  (int ypos, char *texx)
RuntimeScriptValue Sc_DisplayAtY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(DisplayAtY, const char);
}

// void (int msnum)
RuntimeScriptValue Sc_DisplayMessage(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DisplayMessage);
}

// void (int msnum, int ypos)
RuntimeScriptValue Sc_DisplayMessageAtY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(DisplayMessageAtY);
}

// void (int ypos, int ttexcol, int backcol, char *title, int msgnum)
RuntimeScriptValue Sc_DisplayMessageBar(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3_POBJ_PINT(DisplayMessageBar, const char);
}

// void (int ypos, int ttexcol, int backcol, char *title, char*texx, ...)
RuntimeScriptValue Sc_DisplayTopBar(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(DisplayTopBar, 5);
    DisplayTopBar(params[0].IValue, params[1].IValue, params[2].IValue, params[3].Ptr, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

extern RuntimeScriptValue Sc_enable_cursor_mode(const RuntimeScriptValue *params, int32_t param_count);

// void ()
RuntimeScriptValue Sc_EnableGroundLevelAreas(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(EnableGroundLevelAreas);
}

// void (int hsnum)
RuntimeScriptValue Sc_EnableHotspot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(EnableHotspot);
}

// void ()
RuntimeScriptValue Sc_EnableInterface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(EnableInterface);
}

// void (int hsnum)
RuntimeScriptValue Sc_EnableRegion(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(EnableRegion);
}

// int  ()
RuntimeScriptValue Sc_EndCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(EndCutscene);
}

// void (int sppd)
RuntimeScriptValue Sc_FadeIn(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(FadeIn);
}

// void (int spdd)
RuntimeScriptValue Sc_my_fade_out(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(FadeOut);
}

// void (int handle)
RuntimeScriptValue Sc_FileClose(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(FileClose);
}

// int  (int handle)
RuntimeScriptValue Sc_FileIsEOF(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(FileIsEOF);
}

// int (int handle)
RuntimeScriptValue Sc_FileIsError(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(FileIsError);
}

// int (const char*fnmm, const char* cmode)
RuntimeScriptValue Sc_FileOpenCMode(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ2(FileOpenCMode, const char, const char);
}

// void (int handle,char*toread)
RuntimeScriptValue Sc_FileRead(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(FileRead, char);
}

// int (int handle)
RuntimeScriptValue Sc_FileReadInt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(FileReadInt);
}

// char (int handle)
RuntimeScriptValue Sc_FileReadRawChar(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(FileReadRawChar);
}

// int (int handle)
RuntimeScriptValue Sc_FileReadRawInt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(FileReadRawInt);
}

// void (int handle, const char *towrite)
RuntimeScriptValue Sc_FileWrite(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(FileWrite, const char);
}

// void (int handle,int into)
RuntimeScriptValue Sc_FileWriteInt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(FileWriteInt);
}

// void (int handle, int chartoWrite)
RuntimeScriptValue Sc_FileWriteRawChar(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(FileWriteRawChar);
}

// void (int handle, const char*towrite)
RuntimeScriptValue Sc_FileWriteRawLine(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(FileWriteRawLine, const char);
}

// int  (const char* GUIName)
RuntimeScriptValue Sc_FindGUIID(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(FindGUIID, const char);
}

// void (int amount)
RuntimeScriptValue Sc_FlipScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(FlipScreen);
}

// int (SCRIPT_FLOAT(value), int roundDirection)
RuntimeScriptValue Sc_FloatToInt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PFLOAT_PINT(FloatToInt);
}

// int ()
RuntimeScriptValue Sc_GetBackgroundFrame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetBackgroundFrame);
}

// int (int guin, int objn, int ptype)
RuntimeScriptValue Sc_GetButtonPic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(GetButtonPic);
}

// int  (int xx, int yy)
RuntimeScriptValue Sc_GetCharIDAtScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetCharIDAtScreen);
}

extern RuntimeScriptValue Sc_GetCursorMode(const RuntimeScriptValue *params, int32_t param_count);

// int  (int dlg, int opt)
RuntimeScriptValue Sc_GetDialogOption(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetDialogOption);
}

// int  (int opt)
RuntimeScriptValue Sc_GetGameOption(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetGameOption);
}

// int  (int parm, int data1, int data2, int data3)
RuntimeScriptValue Sc_GetGameParameter(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT4(GetGameParameter);
}

// int ()
RuntimeScriptValue Sc_GetGameSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetGameSpeed);
}

// int (int index)
RuntimeScriptValue Sc_GetGlobalInt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetGlobalInt);
}

// int  (int xx,int yy)
RuntimeScriptValue Sc_GetGUIAt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetGUIAt);
}

// int  (int xx, int yy)
RuntimeScriptValue Sc_GetGUIObjectAt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetGUIObjectAt);
}

// int (int xxx,int yyy)
RuntimeScriptValue Sc_GetHotspotIDAtScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetHotspotIDAtScreen);
}

// void (int hotspot, char *buffer)
RuntimeScriptValue Sc_GetHotspotName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(GetHotspotName, char);
}

// int  (int hotspot)
RuntimeScriptValue Sc_GetHotspotPointX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetHotspotPointX);
}

// int  (int hotspot)
RuntimeScriptValue Sc_GetHotspotPointY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetHotspotPointY);
}

// int  (int hss, const char *property)
RuntimeScriptValue Sc_GetHotspotProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT_POBJ(GetHotspotProperty, const char);
}

// void  (int item, const char *property, char *bufer)
RuntimeScriptValue Sc_GetHotspotPropertyText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ2(GetHotspotPropertyText, const char, char);
}

// int  (int xxx, int yyy)
RuntimeScriptValue Sc_GetInvAt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetInvAt);
}

// int (int indx)
RuntimeScriptValue Sc_GetInvGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetInvGraphic);
}

// void (int indx,char*buff)
RuntimeScriptValue Sc_GetInvName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(GetInvName, char);
}

// int  (int item, const char *property)
RuntimeScriptValue Sc_GetInvProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT_POBJ(GetInvProperty, const char);
}

// void  (int item, const char *property, char *bufer)
RuntimeScriptValue Sc_GetInvPropertyText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ2(GetInvPropertyText, const char, char);
}

// int (int xxx,int yyy)
RuntimeScriptValue Sc_GetLocationType(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetLocationType);
}

// int (int xx,int yy)
RuntimeScriptValue Sc_GetObjectIDAtScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetObjectIDAtScreen);
}

// int (int obn)
RuntimeScriptValue Sc_GetObjectBaseline(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetObjectBaseline);
}

// int (int obn)
RuntimeScriptValue Sc_GetObjectGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetObjectGraphic);
}

// void (int obj, char *buffer)
RuntimeScriptValue Sc_GetObjectName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(GetObjectName, char);
}

// int  (int hss, const char *property)
RuntimeScriptValue Sc_GetObjectProperty(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT_POBJ(GetObjectProperty, const char);
}

// void  (int item, const char *property, char *bufer)
RuntimeScriptValue Sc_GetObjectPropertyText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ2(GetObjectPropertyText, const char, char);
}

// int  (int objj)
RuntimeScriptValue Sc_GetObjectX(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetObjectX);
}

// int  (int objj)
RuntimeScriptValue Sc_GetObjectY(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetObjectY);
}

// int ()
RuntimeScriptValue Sc_GetPlayerCharacter(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetPlayerCharacter);
}

// int  ()
RuntimeScriptValue Sc_GetRawTime(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetRawTime);
}

// int  (int xxx, int yyy)
RuntimeScriptValue Sc_GetRegionIDAtRoom(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetRegionIDAtRoom);
}

// int  (int x, int y)
RuntimeScriptValue Sc_GetScalingAt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetScalingAt);
}

// int (int guin,int objn)
RuntimeScriptValue Sc_GetSliderValue(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetSliderValue);
}

// void (int guin, int objn, char*txbuf)
RuntimeScriptValue Sc_GetTextBoxText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(GetTextBoxText, char);
}

// int (char *text, int fontnum, int width)
RuntimeScriptValue Sc_GetTextHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ_PINT2(GetTextHeight, const char);
}

// int (char *text, int fontnum)
RuntimeScriptValue Sc_GetTextWidth(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ_PINT(GetTextWidth, const char);
}

RuntimeScriptValue Sc_GetFontHeight(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetFontHeight);
}

RuntimeScriptValue Sc_GetFontLineSpacing(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetFontLineSpacing);
}

// int (int whatti)
RuntimeScriptValue Sc_sc_GetTime(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(sc_GetTime);
}

// char * (const char *text)
RuntimeScriptValue Sc_get_translation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(char, myScriptStringImpl, get_translation, const char);
}

// int  (char* buffer)
RuntimeScriptValue Sc_GetTranslationName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(GetTranslationName, char);
}

RuntimeScriptValue Sc_GetWalkableAreaAtRoom(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetWalkableAreaAtRoom);
}

// int (int xxx,int yyy)
RuntimeScriptValue Sc_GetWalkableAreaAtScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetWalkableAreaAtScreen);
}

RuntimeScriptValue Sc_GetDrawingSurfaceForWalkableArea(const RuntimeScriptValue *params, int32_t param_count)
{
    (void)params; (void)param_count;
    ScriptDrawingSurface* ret_obj = Room_GetDrawingSurfaceForMask(kRoomAreaWalkable);
    return RuntimeScriptValue().SetDynamicObject(ret_obj, ret_obj);
}

RuntimeScriptValue Sc_GetDrawingSurfaceForWalkbehind(const RuntimeScriptValue *params, int32_t param_count)
{
    (void)params; (void)param_count;
    ScriptDrawingSurface* ret_obj = Room_GetDrawingSurfaceForMask(kRoomAreaWalkBehind);
    return RuntimeScriptValue().SetDynamicObject(ret_obj, ret_obj);
}

// void (int amnt) 
RuntimeScriptValue Sc_GiveScore(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(GiveScore);
}

// int (int roomnum)
RuntimeScriptValue Sc_HasPlayerBeenInRoom(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(HasPlayerBeenInRoom);
}

// void  () 
RuntimeScriptValue Sc_HideMouseCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(HideMouseCursor);
}

// void (const char*msg,char*bufr)
RuntimeScriptValue Sc_sc_inputbox(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(sc_inputbox, const char, char);
}

// void (int ifn)
RuntimeScriptValue Sc_InterfaceOff(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(InterfaceOff);
}

// void (int ifn)
RuntimeScriptValue Sc_InterfaceOn(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(InterfaceOn);
}

// FLOAT_RETURN_TYPE (int value) 
RuntimeScriptValue Sc_IntToFloat(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_FLOAT_PINT(IntToFloat);
}

extern RuntimeScriptValue Sc_IsButtonDown(const RuntimeScriptValue *params, int32_t param_count);

// int ()
RuntimeScriptValue Sc_IsGamePaused(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsGamePaused);
}

// int  (int guinum)
RuntimeScriptValue Sc_IsGUIOn(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsGUIOn);
}

// int  (int xx,int yy,int mood)
RuntimeScriptValue Sc_IsInteractionAvailable(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(IsInteractionAvailable);
}

// int  (int item, int mood)
RuntimeScriptValue Sc_IsInventoryInteractionAvailable(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(IsInventoryInteractionAvailable);
}

// int ()
RuntimeScriptValue Sc_IsInterfaceEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsInterfaceEnabled);
}

// int  (int keycode)
RuntimeScriptValue Sc_IsKeyPressed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsKeyPressed);
}

// int  ()
RuntimeScriptValue Sc_IsMusicVoxAvailable(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsMusicVoxAvailable);
}

// int (int objj)
RuntimeScriptValue Sc_IsObjectAnimating(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsObjectAnimating);
}

// int (int objj)
RuntimeScriptValue Sc_IsObjectMoving(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsObjectMoving);
}

// int  (int objj)
RuntimeScriptValue Sc_IsObjectOn(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsObjectOn);
}

// int (int tnum)
RuntimeScriptValue Sc_IsTimerExpired(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsTimerExpired);
}

// int  ()
RuntimeScriptValue Sc_IsTranslationAvailable(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsTranslationAvailable);
}

// int ()
RuntimeScriptValue Sc_IsVoxAvailable(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsVoxAvailable);
}

// void (int guin, int objn, const char*newitem)
RuntimeScriptValue Sc_ListBoxAdd(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(ListBoxAdd, const char);
}

// void (int guin, int objn)
RuntimeScriptValue Sc_ListBoxClear(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(ListBoxClear);
}

// void  (int guin, int objn, const char*filemask)
RuntimeScriptValue Sc_ListBoxDirList(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(ListBoxDirList, const char);
}

// char* (int guin, int objn, int item, char*buffer)
RuntimeScriptValue Sc_ListBoxGetItemText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3_POBJ(char, myScriptStringImpl, ListBoxGetItemText, char);
}

// int (int guin, int objn)
RuntimeScriptValue Sc_ListBoxGetNumItems(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(ListBoxGetNumItems);
}

// int (int guin, int objn)
RuntimeScriptValue Sc_ListBoxGetSelected(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(ListBoxGetSelected);
}

// void (int guin, int objn, int itemIndex)
RuntimeScriptValue Sc_ListBoxRemove(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ListBoxRemove);
}

// int  (int guin, int objn)
RuntimeScriptValue Sc_ListBoxSaveGameList(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(ListBoxSaveGameList);
}

// void (int guin, int objn, int newsel)
RuntimeScriptValue Sc_ListBoxSetSelected(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ListBoxSetSelected);
}

// void  (int guin, int objn, int item)
RuntimeScriptValue Sc_ListBoxSetTopItem(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ListBoxSetTopItem);
}

// int (const char *filename)
RuntimeScriptValue Sc_LoadImageFile(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(LoadImageFile, const char);
}

// int (int slnum, int width, int height)
RuntimeScriptValue Sc_LoadSaveSlotScreenshot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(LoadSaveSlotScreenshot);
}

// void (int obn)
RuntimeScriptValue Sc_MergeObject(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(MergeObject);
}

// void (int chaa,int hotsp)
RuntimeScriptValue Sc_MoveCharacterToHotspot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(MoveCharacterToHotspot);
}

// void (int objj,int xx,int yy,int spp)
RuntimeScriptValue Sc_MoveObject(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(MoveObject);
}

// void (int objj,int xx,int yy,int spp)
RuntimeScriptValue Sc_MoveObjectDirect(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(MoveObjectDirect);
}

// void (int obn)
RuntimeScriptValue Sc_ObjectOff(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ObjectOff);
}

// void (int obn)
RuntimeScriptValue Sc_ObjectOn(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ObjectOn);
}

extern RuntimeScriptValue Sc_Parser_ParseText(const RuntimeScriptValue *params, int32_t param_count);

// void ()
RuntimeScriptValue Sc_PauseGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(PauseGame);
}

// void (int numb,int playflags)
RuntimeScriptValue Sc_play_flc_file(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(PlayFlic);
}

// void (const char* name, int skip, int flags)
RuntimeScriptValue Sc_scrPlayVideo(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT2(PlayVideo, const char);
}

// void (int dialog)
RuntimeScriptValue Sc_QuitGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(QuitGame);
}

// int (int upto)
RuntimeScriptValue Sc_Rand(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(__Rand);
}

extern RuntimeScriptValue Sc_RefreshMouse(const RuntimeScriptValue *params, int32_t param_count);

// void (int obj)
RuntimeScriptValue Sc_RemoveObjectTint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RemoveObjectTint);
}

// void (int areanum)
RuntimeScriptValue Sc_RemoveWalkableArea(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RemoveWalkableArea);
}

// void (int nrnum)
RuntimeScriptValue Sc_ResetRoom(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ResetRoom);
}

// void ()
RuntimeScriptValue Sc_restart_game(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(restart_game);
}

// void ()
RuntimeScriptValue Sc_restore_game_dialog(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(restore_game_dialog);
}

// void (int slnum)
RuntimeScriptValue Sc_RestoreGameSlot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RestoreGameSlot);
}

// void (int areanum)
RuntimeScriptValue Sc_RestoreWalkableArea(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RestoreWalkableArea);
}

// int  (char *newgame, unsigned int mode, int data)
RuntimeScriptValue Sc_RunAGSGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ_PINT2(RunAGSGame, const char);
}

// void (int tum)
RuntimeScriptValue Sc_RunDialog(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RunDialog);
}

// void  (int hotspothere, int mood)
RuntimeScriptValue Sc_RunHotspotInteraction(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunHotspotInteraction);
}

// void  (int iit, int modd)
RuntimeScriptValue Sc_RunInventoryInteraction(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunInventoryInteraction);
}

// void  (int aa, int mood)
RuntimeScriptValue Sc_RunObjectInteraction(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunObjectInteraction);
}

// void  (int regnum, int mood)
RuntimeScriptValue Sc_RunRegionInteraction(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunRegionInteraction);
}

extern RuntimeScriptValue Sc_Said(const RuntimeScriptValue *params, int32_t param_count);

extern RuntimeScriptValue Sc_SaveCursorForLocationChange(const RuntimeScriptValue *params, int32_t param_count);

// void ()
RuntimeScriptValue Sc_save_game_dialog(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(save_game_dialog);
}

// void (int slotn, const char*descript)
RuntimeScriptValue Sc_save_game(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(save_game, const char);
}

// int (char*namm)
RuntimeScriptValue Sc_SaveScreenShot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(SaveScreenShot, const char);
}

// void  (int red, int green, int blue, int opacity, int luminance)
RuntimeScriptValue Sc_SetAmbientTint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(SetAmbientTint);
}

RuntimeScriptValue Sc_SetAmbientLightLevel(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetAmbientLightLevel);
}

// void (int area, int brightness)
RuntimeScriptValue Sc_SetAreaLightLevel(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetAreaLightLevel);
}

// void (int area, int min, int max)
RuntimeScriptValue Sc_SetAreaScaling(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetAreaScaling);
}

// void (int frnum)
RuntimeScriptValue Sc_SetBackgroundFrame(const RuntimeScriptValue *params, int32_t param_count)
{
     API_SCALL_VOID_PINT(SetBackgroundFrame);
}

// void (int guin,int objn,int ptype,int slotn)
RuntimeScriptValue Sc_SetButtonPic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetButtonPic);
}

// void (int guin,int objn,char*newtx)
RuntimeScriptValue Sc_SetButtonText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(SetButtonText, const char);
}

extern RuntimeScriptValue Sc_set_cursor_mode(const RuntimeScriptValue *params, int32_t param_count);
extern RuntimeScriptValue Sc_set_default_cursor(const RuntimeScriptValue *params, int32_t param_count);

// void (int dlg,int opt,int onoroff)
RuntimeScriptValue Sc_SetDialogOption(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetDialogOption);
}

// void (int red, int green, int blue)
RuntimeScriptValue Sc_SetFadeColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetFadeColor);
}

// int  (int opt, int setting)
RuntimeScriptValue Sc_SetGameOption(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(SetGameOption);
}

// void (int newspd)
RuntimeScriptValue Sc_SetGameSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetGameSpeed);
}

// void (int index,int valu)
RuntimeScriptValue Sc_SetGlobalInt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGlobalInt);
}

// void  (int guin, int slotn)
RuntimeScriptValue Sc_SetGUIBackgroundPic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGUIBackgroundPic);
}

// void (int guin, int clickable)
RuntimeScriptValue Sc_SetGUIClickable(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGUIClickable);
}

// void (int guin, int objn, int enabled)
RuntimeScriptValue Sc_SetGUIObjectEnabled(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetGUIObjectEnabled);
}

// void (int guin, int objn, int xx, int yy)
RuntimeScriptValue Sc_SetGUIObjectPosition(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetGUIObjectPosition);
}

// void (int ifn, int objn, int newwid, int newhit)
RuntimeScriptValue Sc_SetGUIObjectSize(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetGUIObjectSize);
}

// void (int ifn,int xx,int yy)
RuntimeScriptValue Sc_SetGUIPosition(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetGUIPosition);
}

// void  (int ifn, int widd, int hitt)
RuntimeScriptValue Sc_SetGUISize(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetGUISize);
}

// void (int ifn, int trans)
RuntimeScriptValue Sc_SetGUITransparency(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGUITransparency);
}

// void (int guin, int z)
RuntimeScriptValue Sc_SetGUIZOrder(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGUIZOrder);
}

// void (int invi, const char *newName)
RuntimeScriptValue Sc_SetInvItemName(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(SetInvItemName, const char);
}

// void (int invi, int piccy)
RuntimeScriptValue Sc_set_inv_item_pic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(set_inv_item_pic);
}

// void (int guin,int objn, int colr)
RuntimeScriptValue Sc_SetLabelColor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetLabelColor);
}

// void (int guin,int objn, int fontnum)
RuntimeScriptValue Sc_SetLabelFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetLabelFont);
}

// void (int guin,int objn,char*newtx)
RuntimeScriptValue Sc_SetLabelText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(SetLabelText, const char);
}

extern RuntimeScriptValue Sc_SetMouseBounds(const RuntimeScriptValue *params, int32_t param_count);
extern RuntimeScriptValue Sc_set_mouse_cursor(const RuntimeScriptValue *params, int32_t param_count);
extern RuntimeScriptValue Sc_SetMousePosition(const RuntimeScriptValue *params, int32_t param_count);

// void  (int mode)
RuntimeScriptValue Sc_SetMultitasking(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetMultitasking);
}

extern RuntimeScriptValue Sc_SetNextCursor(const RuntimeScriptValue *params, int32_t param_count);

// void (int newtrans)
RuntimeScriptValue Sc_SetNextScreenTransition(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetNextScreenTransition);
}

extern RuntimeScriptValue Sc_SetNormalFont(const RuntimeScriptValue *params, int32_t param_count);

// void  (int obn, int basel)
RuntimeScriptValue Sc_SetObjectBaseline(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectBaseline);
}

// void  (int cha, int clik)
RuntimeScriptValue Sc_SetObjectClickable(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectClickable);
}

// void (int obn,int viw,int lop,int fra)
RuntimeScriptValue Sc_SetObjectFrame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetObjectFrame);
}

// void (int obn,int slott) 
RuntimeScriptValue Sc_SetObjectGraphic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectGraphic);
}

// void  (int cha, int clik)
RuntimeScriptValue Sc_SetObjectIgnoreWalkbehinds(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectIgnoreWalkbehinds);
}

// void (int objj, int tox, int toy)
RuntimeScriptValue Sc_SetObjectPosition(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetObjectPosition);
}

// void (int obj, int red, int green, int blue, int opacity, int luminance)
RuntimeScriptValue Sc_SetObjectTint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT6(SetObjectTint);
}

// void (int obn,int trans)
RuntimeScriptValue Sc_SetObjectTransparency(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectTransparency);
}

// void (int obn,int vii)
RuntimeScriptValue Sc_SetObjectView(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectView);
}

// void (int inndx,int rr,int gg,int bb)
RuntimeScriptValue Sc_SetPalRGB(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetPalRGB);
}

// void  (int area, int red, int green, int blue, int amount)
RuntimeScriptValue Sc_SetRegionTint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(SetRegionTint);
}

// void ()
RuntimeScriptValue Sc_SetRestartPoint(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SetRestartPoint);
}

// void (int newtrans)
RuntimeScriptValue Sc_SetScreenTransition(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetScreenTransition);
}

// void (int guin,int objn, int valn)
RuntimeScriptValue Sc_SetSliderValue(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetSliderValue);
}

extern RuntimeScriptValue Sc_SetSpeechFont(const RuntimeScriptValue *params, int32_t param_count);

// void (int newvol)
RuntimeScriptValue Sc_SetSpeechVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSpeechVolume);
}

// void (int guin,int objn, int fontnum)
RuntimeScriptValue Sc_SetTextBoxFont(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetTextBoxFont);
}

// void (int guin, int objn, char*txbuf)
RuntimeScriptValue Sc_SetTextBoxText(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(SetTextBoxText, const char);
}

// void  (int guinum)
RuntimeScriptValue Sc_SetTextWindowGUI(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetTextWindowGUI);
}

// void (int tnum,int timeout)
RuntimeScriptValue Sc_script_SetTimer(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(script_SetTimer);
}

// void (int wa,int bl)
RuntimeScriptValue Sc_SetWalkBehindBase(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetWalkBehindBase);
}

// void (int severe)
RuntimeScriptValue Sc_ShakeScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ShakeScreen);
}

// void  (int delay, int amount, int length)
RuntimeScriptValue Sc_ShakeScreenBackground(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ShakeScreenBackground);
}

// void  ()
RuntimeScriptValue Sc_ShowMouseCursor(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(ShowMouseCursor);
}

RuntimeScriptValue Sc_SkipCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SkipCutscene);
}

// void (int cc)
RuntimeScriptValue Sc_SkipUntilCharacterStops(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SkipUntilCharacterStops);
}

// void  (int skipwith)
RuntimeScriptValue Sc_StartCutscene(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StartCutscene);
}

// void  (int keyToStop)
RuntimeScriptValue Sc_scStartRecording(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(scStartRecording);
}

// void  (int chid) 
RuntimeScriptValue Sc_stop_and_destroy_channel(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(stop_and_destroy_channel);
}

// void ()
RuntimeScriptValue Sc_StopDialog(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(StopDialog);
}

// void (int chaa)
RuntimeScriptValue Sc_StopMoving(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StopMoving);
}

// void (int objj)
RuntimeScriptValue Sc_StopObjectMoving(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StopObjectMoving);
}

// int (const char*stino)
RuntimeScriptValue Sc_StringToInt(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(StringToInt, const char);
}

RuntimeScriptValue Sc_strlen(const RuntimeScriptValue *params, int32_t param_count)
{
    // Calling C stdlib function strlen
    API_SCALL_INT_POBJ(strlen, const char);
}

// void (int red, int grn, int blu)
RuntimeScriptValue Sc_TintScreen(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(TintScreen);
}

// void ()
RuntimeScriptValue Sc_UnPauseGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(UnPauseGame);
}

// void ()
RuntimeScriptValue Sc_update_invorder(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(update_invorder);
}

// void ()
RuntimeScriptValue Sc_UpdatePalette(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(UpdatePalette);
}

// void (int nloops)
RuntimeScriptValue Sc_scrWait(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(scrWait);
}

// int (int nloops)
RuntimeScriptValue Sc_WaitKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(WaitKey);
}

RuntimeScriptValue Sc_WaitMouse(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(WaitMouse);
}

// int (int nloops)
RuntimeScriptValue Sc_WaitMouseKey(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(WaitMouseKey);
}

// int (int input_flags, int nloops)
RuntimeScriptValue Sc_WaitInput(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(WaitInput);
}

RuntimeScriptValue Sc_SkipWait(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SkipWait);
}

//=============================================================================
//
// Exclusive API for Plugins
//
//=============================================================================

// void (char*texx, ...)
void ScPl_sc_AbortGame(const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    _sc_AbortGame(scsf_buffer);
}

// void (char*texx, ...)
void ScPl_Display(char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    DisplaySimple(scsf_buffer);
}

// void (int xxp,int yyp,int widd,char*texx, ...)
void ScPl_DisplayAt(int xxp, int yyp, int widd, char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    DisplayAt(xxp, yyp, widd, scsf_buffer);
}

// void (int ypos, int ttexcol, int backcol, char *title, char*texx, ...)
void ScPl_DisplayTopBar(int ypos, int ttexcol, int backcol, char *title, char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    DisplayTopBar(ypos, ttexcol, backcol, title, scsf_buffer);
}


void RegisterGlobalAPI()
{
    ccAddExternalStaticFunction("AbortGame",                Sc_sc_AbortGame);
	ccAddExternalStaticFunction("AnimateButton",            Sc_AnimateButton);
	ccAddExternalStaticFunction("AreThingsOverlapping",     Sc_AreThingsOverlapping);
	ccAddExternalStaticFunction("CallRoomScript",           Sc_CallRoomScript);
	ccAddExternalStaticFunction("CentreGUI",                Sc_CentreGUI);
	ccAddExternalStaticFunction("ChangeCursorGraphic",      Sc_ChangeCursorGraphic);
	ccAddExternalStaticFunction("ChangeCursorHotspot",      Sc_ChangeCursorHotspot);
	ccAddExternalStaticFunction("ClaimEvent",               Sc_ClaimEvent);
	ccAddExternalStaticFunction("CyclePalette",             Sc_CyclePalette);
	ccAddExternalStaticFunction("Debug",                    Sc_script_debug);
	ccAddExternalStaticFunction("DeleteSaveSlot",           Sc_DeleteSaveSlot);
	ccAddExternalStaticFunction("DeleteSprite",             Sc_free_dynamic_sprite);
	ccAddExternalStaticFunction("DisableCursorMode",        Sc_disable_cursor_mode);
	ccAddExternalStaticFunction("DisableGroundLevelAreas",  Sc_DisableGroundLevelAreas);
	ccAddExternalStaticFunction("DisableHotspot",           Sc_DisableHotspot);
	ccAddExternalStaticFunction("DisableInterface",         Sc_DisableInterface);
	ccAddExternalStaticFunction("DisableRegion",            Sc_DisableRegion);
	ccAddExternalStaticFunction("Display",                  Sc_Display);
	ccAddExternalStaticFunction("DisplayAt",                Sc_DisplayAt);
	ccAddExternalStaticFunction("DisplayAtY",               Sc_DisplayAtY);
	ccAddExternalStaticFunction("DisplayMessage",           Sc_DisplayMessage);
	ccAddExternalStaticFunction("DisplayMessageAtY",        Sc_DisplayMessageAtY);
	ccAddExternalStaticFunction("DisplayMessageBar",        Sc_DisplayMessageBar);
	ccAddExternalStaticFunction("DisplayTopBar",            Sc_DisplayTopBar);
	ccAddExternalStaticFunction("EnableCursorMode",         Sc_enable_cursor_mode);
	ccAddExternalStaticFunction("EnableGroundLevelAreas",   Sc_EnableGroundLevelAreas);
	ccAddExternalStaticFunction("EnableHotspot",            Sc_EnableHotspot);
	ccAddExternalStaticFunction("EnableInterface",          Sc_EnableInterface);
	ccAddExternalStaticFunction("EnableRegion",             Sc_EnableRegion);
	ccAddExternalStaticFunction("EndCutscene",              Sc_EndCutscene);
	ccAddExternalStaticFunction("FadeIn",                   Sc_FadeIn);
	ccAddExternalStaticFunction("FadeOut",                  Sc_my_fade_out);
	ccAddExternalStaticFunction("FileClose",                Sc_FileClose);
	ccAddExternalStaticFunction("FileIsEOF",                Sc_FileIsEOF);
	ccAddExternalStaticFunction("FileIsError",              Sc_FileIsError);
    // NOTE: FileOpenCMode is a backwards-compatible replacement for old-style global script function FileOpen
	ccAddExternalStaticFunction("FileOpen",                 Sc_FileOpenCMode);
	ccAddExternalStaticFunction("FileRead",                 Sc_FileRead);
	ccAddExternalStaticFunction("FileReadInt",              Sc_FileReadInt);
	ccAddExternalStaticFunction("FileReadRawChar",          Sc_FileReadRawChar);
	ccAddExternalStaticFunction("FileReadRawInt",           Sc_FileReadRawInt);
	ccAddExternalStaticFunction("FileWrite",                Sc_FileWrite);
	ccAddExternalStaticFunction("FileWriteInt",             Sc_FileWriteInt);
	ccAddExternalStaticFunction("FileWriteRawChar",         Sc_FileWriteRawChar);
	ccAddExternalStaticFunction("FileWriteRawLine",         Sc_FileWriteRawLine);
	ccAddExternalStaticFunction("FindGUIID",                Sc_FindGUIID);
	ccAddExternalStaticFunction("FlipScreen",               Sc_FlipScreen);
	ccAddExternalStaticFunction("FloatToInt",               Sc_FloatToInt);
	ccAddExternalStaticFunction("GetBackgroundFrame",       Sc_GetBackgroundFrame);
	ccAddExternalStaticFunction("GetButtonPic",             Sc_GetButtonPic);
	ccAddExternalStaticFunction("GetCharacterAt",           Sc_GetCharIDAtScreen);
	ccAddExternalStaticFunction("GetCursorMode",            Sc_GetCursorMode);
	ccAddExternalStaticFunction("GetDialogOption",          Sc_GetDialogOption);
	ccAddExternalStaticFunction("GetGameOption",            Sc_GetGameOption);
	ccAddExternalStaticFunction("GetGameParameter",         Sc_GetGameParameter);
	ccAddExternalStaticFunction("GetGameSpeed",             Sc_GetGameSpeed);
	ccAddExternalStaticFunction("GetGlobalInt",             Sc_GetGlobalInt);
	ccAddExternalStaticFunction("GetGUIAt",                 Sc_GetGUIAt);
	ccAddExternalStaticFunction("GetGUIObjectAt",           Sc_GetGUIObjectAt);
	ccAddExternalStaticFunction("GetHotspotAt",             Sc_GetHotspotIDAtScreen);
	ccAddExternalStaticFunction("GetHotspotName",           Sc_GetHotspotName);
	ccAddExternalStaticFunction("GetHotspotPointX",         Sc_GetHotspotPointX);
	ccAddExternalStaticFunction("GetHotspotPointY",         Sc_GetHotspotPointY);
	ccAddExternalStaticFunction("GetHotspotProperty",       Sc_GetHotspotProperty);
	ccAddExternalStaticFunction("GetHotspotPropertyText",   Sc_GetHotspotPropertyText);
	ccAddExternalStaticFunction("GetInvAt",                 Sc_GetInvAt);
	ccAddExternalStaticFunction("GetInvGraphic",            Sc_GetInvGraphic);
	ccAddExternalStaticFunction("GetInvName",               Sc_GetInvName);
	ccAddExternalStaticFunction("GetInvProperty",           Sc_GetInvProperty);
	ccAddExternalStaticFunction("GetInvPropertyText",       Sc_GetInvPropertyText);
	ccAddExternalStaticFunction("GetLocationType",          Sc_GetLocationType);
	ccAddExternalStaticFunction("GetObjectAt",              Sc_GetObjectIDAtScreen);
	ccAddExternalStaticFunction("GetObjectBaseline",        Sc_GetObjectBaseline);
	ccAddExternalStaticFunction("GetObjectGraphic",         Sc_GetObjectGraphic);
	ccAddExternalStaticFunction("GetObjectName",            Sc_GetObjectName);
	ccAddExternalStaticFunction("GetObjectProperty",        Sc_GetObjectProperty);
	ccAddExternalStaticFunction("GetObjectPropertyText",    Sc_GetObjectPropertyText);
	ccAddExternalStaticFunction("GetObjectX",               Sc_GetObjectX);
	ccAddExternalStaticFunction("GetObjectY",               Sc_GetObjectY);
	ccAddExternalStaticFunction("GetPlayerCharacter",       Sc_GetPlayerCharacter);
	ccAddExternalStaticFunction("GetRawTime",               Sc_GetRawTime);
	ccAddExternalStaticFunction("GetRegionAt",              Sc_GetRegionIDAtRoom);
	ccAddExternalStaticFunction("GetRoomProperty",          Sc_Room_GetProperty);
	ccAddExternalStaticFunction("GetScalingAt",             Sc_GetScalingAt);
	ccAddExternalStaticFunction("GetSliderValue",           Sc_GetSliderValue);
	ccAddExternalStaticFunction("GetTextBoxText",           Sc_GetTextBoxText);
	ccAddExternalStaticFunction("GetTextHeight",            Sc_GetTextHeight);
	ccAddExternalStaticFunction("GetTextWidth",             Sc_GetTextWidth);
	ccAddExternalStaticFunction("GetFontHeight",            Sc_GetFontHeight);
	ccAddExternalStaticFunction("GetFontLineSpacing",       Sc_GetFontLineSpacing);
	ccAddExternalStaticFunction("GetTime",                  Sc_sc_GetTime);
	ccAddExternalStaticFunction("GetTranslation",           Sc_get_translation);
	ccAddExternalStaticFunction("GetTranslationName",       Sc_GetTranslationName);
	ccAddExternalStaticFunction("GetWalkableAreaAtRoom",    Sc_GetWalkableAreaAtRoom);
    ccAddExternalStaticFunction("GetWalkableAreaAtScreen",  Sc_GetWalkableAreaAtScreen);
    ccAddExternalStaticFunction("GetDrawingSurfaceForWalkableArea", Sc_GetDrawingSurfaceForWalkableArea);
    ccAddExternalStaticFunction("GetDrawingSurfaceForWalkbehind", Sc_GetDrawingSurfaceForWalkbehind);
	ccAddExternalStaticFunction("GiveScore",                Sc_GiveScore);
	ccAddExternalStaticFunction("HasPlayerBeenInRoom",      Sc_HasPlayerBeenInRoom);
	ccAddExternalStaticFunction("HideMouseCursor",          Sc_HideMouseCursor);
	ccAddExternalStaticFunction("InputBox",                 Sc_sc_inputbox);
	ccAddExternalStaticFunction("InterfaceOff",             Sc_InterfaceOff);
	ccAddExternalStaticFunction("InterfaceOn",              Sc_InterfaceOn);
	ccAddExternalStaticFunction("IntToFloat",               Sc_IntToFloat);
	ccAddExternalStaticFunction("IsButtonDown",             Sc_IsButtonDown);
	ccAddExternalStaticFunction("IsGamePaused",             Sc_IsGamePaused);
	ccAddExternalStaticFunction("IsGUIOn",                  Sc_IsGUIOn);
	ccAddExternalStaticFunction("IsInteractionAvailable",   Sc_IsInteractionAvailable);
	ccAddExternalStaticFunction("IsInventoryInteractionAvailable", Sc_IsInventoryInteractionAvailable);
	ccAddExternalStaticFunction("IsInterfaceEnabled",       Sc_IsInterfaceEnabled);
	ccAddExternalStaticFunction("IsKeyPressed",             Sc_IsKeyPressed);
	ccAddExternalStaticFunction("IsMusicVoxAvailable",      Sc_IsMusicVoxAvailable);
	ccAddExternalStaticFunction("IsObjectAnimating",        Sc_IsObjectAnimating);
	ccAddExternalStaticFunction("IsObjectMoving",           Sc_IsObjectMoving);
	ccAddExternalStaticFunction("IsObjectOn",               Sc_IsObjectOn);
	ccAddExternalStaticFunction("IsTimerExpired",           Sc_IsTimerExpired);
	ccAddExternalStaticFunction("IsTranslationAvailable",   Sc_IsTranslationAvailable);
	ccAddExternalStaticFunction("IsVoxAvailable",           Sc_IsVoxAvailable);
	ccAddExternalStaticFunction("ListBoxAdd",               Sc_ListBoxAdd);
	ccAddExternalStaticFunction("ListBoxClear",             Sc_ListBoxClear);
	ccAddExternalStaticFunction("ListBoxDirList",           Sc_ListBoxDirList);
	ccAddExternalStaticFunction("ListBoxGetItemText",       Sc_ListBoxGetItemText);
	ccAddExternalStaticFunction("ListBoxGetNumItems",       Sc_ListBoxGetNumItems);
	ccAddExternalStaticFunction("ListBoxGetSelected",       Sc_ListBoxGetSelected);
	ccAddExternalStaticFunction("ListBoxRemove",            Sc_ListBoxRemove);
	ccAddExternalStaticFunction("ListBoxSaveGameList",      Sc_ListBoxSaveGameList);
	ccAddExternalStaticFunction("ListBoxSetSelected",       Sc_ListBoxSetSelected);
	ccAddExternalStaticFunction("ListBoxSetTopItem",        Sc_ListBoxSetTopItem);
	ccAddExternalStaticFunction("LoadImageFile",            Sc_LoadImageFile);
	ccAddExternalStaticFunction("LoadSaveSlotScreenshot",   Sc_LoadSaveSlotScreenshot);
	ccAddExternalStaticFunction("MergeObject",              Sc_MergeObject);
	ccAddExternalStaticFunction("MoveCharacterToHotspot",   Sc_MoveCharacterToHotspot);
	ccAddExternalStaticFunction("MoveObject",               Sc_MoveObject);
	ccAddExternalStaticFunction("MoveObjectDirect",         Sc_MoveObjectDirect);
	ccAddExternalStaticFunction("ObjectOff",                Sc_ObjectOff);
	ccAddExternalStaticFunction("ObjectOn",                 Sc_ObjectOn);
	ccAddExternalStaticFunction("PauseGame",                Sc_PauseGame);
	ccAddExternalStaticFunction("PlayFlic",                 Sc_play_flc_file);
	ccAddExternalStaticFunction("PlayVideo",                Sc_scrPlayVideo);
	ccAddExternalStaticFunction("QuitGame",                 Sc_QuitGame);
	ccAddExternalStaticFunction("Random",                   Sc_Rand);
	ccAddExternalStaticFunction("RefreshMouse",             Sc_RefreshMouse);
	ccAddExternalStaticFunction("RemoveObjectTint",         Sc_RemoveObjectTint);
	ccAddExternalStaticFunction("RemoveWalkableArea",       Sc_RemoveWalkableArea);
	ccAddExternalStaticFunction("ResetRoom",                Sc_ResetRoom);
	ccAddExternalStaticFunction("RestartGame",              Sc_restart_game);
	ccAddExternalStaticFunction("RestoreGameDialog",        Sc_restore_game_dialog);
	ccAddExternalStaticFunction("RestoreGameSlot",          Sc_RestoreGameSlot);
	ccAddExternalStaticFunction("RestoreWalkableArea",      Sc_RestoreWalkableArea);
	ccAddExternalStaticFunction("RunAGSGame",               Sc_RunAGSGame);
	ccAddExternalStaticFunction("RunDialog",                Sc_RunDialog);
	ccAddExternalStaticFunction("RunHotspotInteraction",    Sc_RunHotspotInteraction);
	ccAddExternalStaticFunction("RunInventoryInteraction",  Sc_RunInventoryInteraction);
	ccAddExternalStaticFunction("RunObjectInteraction",     Sc_RunObjectInteraction);
	ccAddExternalStaticFunction("RunRegionInteraction",     Sc_RunRegionInteraction);
	ccAddExternalStaticFunction("Said",                     Sc_Said);
	ccAddExternalStaticFunction("SaveCursorForLocationChange", Sc_SaveCursorForLocationChange);
	ccAddExternalStaticFunction("SaveGameDialog",           Sc_save_game_dialog);
	ccAddExternalStaticFunction("SaveGameSlot",             Sc_save_game);
	ccAddExternalStaticFunction("SaveScreenShot",           Sc_SaveScreenShot);
	ccAddExternalStaticFunction("SetAmbientTint",           Sc_SetAmbientTint);
    ccAddExternalStaticFunction("SetAmbientLightLevel",     Sc_SetAmbientLightLevel);
	ccAddExternalStaticFunction("SetAreaLightLevel",        Sc_SetAreaLightLevel);
	ccAddExternalStaticFunction("SetAreaScaling",           Sc_SetAreaScaling);
	ccAddExternalStaticFunction("SetBackgroundFrame",       Sc_SetBackgroundFrame);
	ccAddExternalStaticFunction("SetButtonPic",             Sc_SetButtonPic);
	ccAddExternalStaticFunction("SetButtonText",            Sc_SetButtonText);
	ccAddExternalStaticFunction("SetCursorMode",            Sc_set_cursor_mode);
	ccAddExternalStaticFunction("SetDefaultCursor",         Sc_set_default_cursor);
	ccAddExternalStaticFunction("SetDialogOption",          Sc_SetDialogOption);
	ccAddExternalStaticFunction("SetFadeColor",             Sc_SetFadeColor);
	ccAddExternalStaticFunction("SetGameOption",            Sc_SetGameOption);
	ccAddExternalStaticFunction("SetGameSpeed",             Sc_SetGameSpeed);
	ccAddExternalStaticFunction("SetGlobalInt",             Sc_SetGlobalInt);
	ccAddExternalStaticFunction("SetGUIBackgroundPic",      Sc_SetGUIBackgroundPic);
	ccAddExternalStaticFunction("SetGUIClickable",          Sc_SetGUIClickable);
	ccAddExternalStaticFunction("SetGUIObjectEnabled",      Sc_SetGUIObjectEnabled);
	ccAddExternalStaticFunction("SetGUIObjectPosition",     Sc_SetGUIObjectPosition);
	ccAddExternalStaticFunction("SetGUIObjectSize",         Sc_SetGUIObjectSize);
	ccAddExternalStaticFunction("SetGUIPosition",           Sc_SetGUIPosition);
	ccAddExternalStaticFunction("SetGUISize",               Sc_SetGUISize);
	ccAddExternalStaticFunction("SetGUITransparency",       Sc_SetGUITransparency);
	ccAddExternalStaticFunction("SetGUIZOrder",             Sc_SetGUIZOrder);
	ccAddExternalStaticFunction("SetInvItemName",           Sc_SetInvItemName);
	ccAddExternalStaticFunction("SetInvItemPic",            Sc_set_inv_item_pic);
	ccAddExternalStaticFunction("SetLabelColor",            Sc_SetLabelColor);
	ccAddExternalStaticFunction("SetLabelFont",             Sc_SetLabelFont);
	ccAddExternalStaticFunction("SetLabelText",             Sc_SetLabelText);
	ccAddExternalStaticFunction("SetMouseBounds",           Sc_SetMouseBounds);
	ccAddExternalStaticFunction("SetMouseCursor",           Sc_set_mouse_cursor);
	ccAddExternalStaticFunction("SetMousePosition",         Sc_SetMousePosition);
	ccAddExternalStaticFunction("SetMultitaskingMode",      Sc_SetMultitasking);
	ccAddExternalStaticFunction("SetNextCursorMode",        Sc_SetNextCursor);
	ccAddExternalStaticFunction("SetNextScreenTransition",  Sc_SetNextScreenTransition);
	ccAddExternalStaticFunction("SetNormalFont",            Sc_SetNormalFont);
	ccAddExternalStaticFunction("SetObjectBaseline",        Sc_SetObjectBaseline);
	ccAddExternalStaticFunction("SetObjectClickable",       Sc_SetObjectClickable);
	ccAddExternalStaticFunction("SetObjectFrame",           Sc_SetObjectFrame);
	ccAddExternalStaticFunction("SetObjectGraphic",         Sc_SetObjectGraphic);
	ccAddExternalStaticFunction("SetObjectIgnoreWalkbehinds", Sc_SetObjectIgnoreWalkbehinds);
	ccAddExternalStaticFunction("SetObjectPosition",        Sc_SetObjectPosition);
	ccAddExternalStaticFunction("SetObjectTint",            Sc_SetObjectTint);
	ccAddExternalStaticFunction("SetObjectTransparency",    Sc_SetObjectTransparency);
	ccAddExternalStaticFunction("SetObjectView",            Sc_SetObjectView);
	ccAddExternalStaticFunction("SetPalRGB",                Sc_SetPalRGB);
	ccAddExternalStaticFunction("SetRegionTint",            Sc_SetRegionTint);
	ccAddExternalStaticFunction("SetRestartPoint",          Sc_SetRestartPoint);
	ccAddExternalStaticFunction("SetScreenTransition",      Sc_SetScreenTransition);
	ccAddExternalStaticFunction("SetSliderValue",           Sc_SetSliderValue);
	ccAddExternalStaticFunction("SetSpeechFont",            Sc_SetSpeechFont);
	ccAddExternalStaticFunction("SetSpeechVolume",          Sc_SetSpeechVolume);
	ccAddExternalStaticFunction("SetTextBoxFont",           Sc_SetTextBoxFont);
	ccAddExternalStaticFunction("SetTextBoxText",           Sc_SetTextBoxText);
	ccAddExternalStaticFunction("SetTextWindowGUI",         Sc_SetTextWindowGUI);
	ccAddExternalStaticFunction("SetTimer",                 Sc_script_SetTimer);
	ccAddExternalStaticFunction("SetWalkBehindBase",        Sc_SetWalkBehindBase);
	ccAddExternalStaticFunction("ShakeScreen",              Sc_ShakeScreen);
	ccAddExternalStaticFunction("ShakeScreenBackground",    Sc_ShakeScreenBackground);
	ccAddExternalStaticFunction("ShowMouseCursor",          Sc_ShowMouseCursor);
    ccAddExternalStaticFunction("SkipCutscene",             Sc_SkipCutscene);
	ccAddExternalStaticFunction("SkipUntilCharacterStops",  Sc_SkipUntilCharacterStops);
	ccAddExternalStaticFunction("StartCutscene",            Sc_StartCutscene);
	ccAddExternalStaticFunction("StartRecording",           Sc_scStartRecording);
	ccAddExternalStaticFunction("StopChannel",              Sc_stop_and_destroy_channel);
	ccAddExternalStaticFunction("StopDialog",               Sc_StopDialog);
	ccAddExternalStaticFunction("StopMoving",               Sc_StopMoving);
	ccAddExternalStaticFunction("StopObjectMoving",         Sc_StopObjectMoving);
	ccAddExternalStaticFunction("TintScreen",               Sc_TintScreen);
	ccAddExternalStaticFunction("UnPauseGame",              Sc_UnPauseGame);
	ccAddExternalStaticFunction("UpdateInventory",          Sc_update_invorder);
	ccAddExternalStaticFunction("UpdatePalette",            Sc_UpdatePalette);
	ccAddExternalStaticFunction("Wait",                     Sc_scrWait);
	ccAddExternalStaticFunction("WaitKey",                  Sc_WaitKey);
	ccAddExternalStaticFunction("WaitMouse",                Sc_WaitMouse);
	ccAddExternalStaticFunction("WaitMouseKey",             Sc_WaitMouseKey);
	ccAddExternalStaticFunction("WaitInput",                Sc_WaitInput);
	ccAddExternalStaticFunction("SkipWait",                 Sc_SkipWait);

    /* ----------------------- Registering unsafe exports for plugins -----------------------*/

    ccAddExternalFunctionForPlugin("AbortGame",                (void*)ScPl_sc_AbortGame);
    ccAddExternalFunctionForPlugin("AnimateButton",            (void*)AnimateButton);
    ccAddExternalFunctionForPlugin("AreThingsOverlapping",     (void*)AreThingsOverlapping);
    ccAddExternalFunctionForPlugin("CallRoomScript",           (void*)CallRoomScript);
    ccAddExternalFunctionForPlugin("CentreGUI",                (void*)CentreGUI);
    ccAddExternalFunctionForPlugin("ChangeCursorGraphic",      (void*)ChangeCursorGraphic);
    ccAddExternalFunctionForPlugin("ChangeCursorHotspot",      (void*)ChangeCursorHotspot);
    ccAddExternalFunctionForPlugin("ClaimEvent",               (void*)ClaimEvent);
    ccAddExternalFunctionForPlugin("CyclePalette",             (void*)CyclePalette);
    ccAddExternalFunctionForPlugin("Debug",                    (void*)script_debug);
    ccAddExternalFunctionForPlugin("DeleteSaveSlot",           (void*)DeleteSaveSlot);
    ccAddExternalFunctionForPlugin("DeleteSprite",             (void*)free_dynamic_sprite);
    ccAddExternalFunctionForPlugin("DisableCursorMode",        (void*)disable_cursor_mode);
    ccAddExternalFunctionForPlugin("DisableGroundLevelAreas",  (void*)DisableGroundLevelAreas);
    ccAddExternalFunctionForPlugin("DisableHotspot",           (void*)DisableHotspot);
    ccAddExternalFunctionForPlugin("DisableInterface",         (void*)DisableInterface);
    ccAddExternalFunctionForPlugin("DisableRegion",            (void*)DisableRegion);
    ccAddExternalFunctionForPlugin("Display",                  (void*)ScPl_Display);
    ccAddExternalFunctionForPlugin("DisplayAt",                (void*)ScPl_DisplayAt);
    ccAddExternalFunctionForPlugin("DisplayAtY",               (void*)DisplayAtY);
    ccAddExternalFunctionForPlugin("DisplayMessage",           (void*)DisplayMessage);
    ccAddExternalFunctionForPlugin("DisplayMessageAtY",        (void*)DisplayMessageAtY);
    ccAddExternalFunctionForPlugin("DisplayMessageBar",        (void*)DisplayMessageBar);
    ccAddExternalFunctionForPlugin("DisplayTopBar",            (void*)ScPl_DisplayTopBar);
    ccAddExternalFunctionForPlugin("EnableCursorMode",         (void*)enable_cursor_mode);
    ccAddExternalFunctionForPlugin("EnableGroundLevelAreas",   (void*)EnableGroundLevelAreas);
    ccAddExternalFunctionForPlugin("EnableHotspot",            (void*)EnableHotspot);
    ccAddExternalFunctionForPlugin("EnableInterface",          (void*)EnableInterface);
    ccAddExternalFunctionForPlugin("EnableRegion",             (void*)EnableRegion);
    ccAddExternalFunctionForPlugin("EndCutscene",              (void*)EndCutscene);
    ccAddExternalFunctionForPlugin("FadeIn",                   (void*)FadeIn);
    ccAddExternalFunctionForPlugin("FadeOut",                  (void*)FadeOut);
    ccAddExternalFunctionForPlugin("FileClose",                (void*)FileClose);
    ccAddExternalFunctionForPlugin("FileIsEOF",                (void*)FileIsEOF);
    ccAddExternalFunctionForPlugin("FileIsError",              (void*)FileIsError);
    // NOTE: FileOpenCMode is a backwards-compatible replacement for old-style global script function FileOpen
    ccAddExternalFunctionForPlugin("FileOpen",                 (void*)FileOpenCMode);
    ccAddExternalFunctionForPlugin("FileRead",                 (void*)FileRead);
    ccAddExternalFunctionForPlugin("FileReadInt",              (void*)FileReadInt);
    ccAddExternalFunctionForPlugin("FileReadRawChar",          (void*)FileReadRawChar);
    ccAddExternalFunctionForPlugin("FileReadRawInt",           (void*)FileReadRawInt);
    ccAddExternalFunctionForPlugin("FileWrite",                (void*)FileWrite);
    ccAddExternalFunctionForPlugin("FileWriteInt",             (void*)FileWriteInt);
    ccAddExternalFunctionForPlugin("FileWriteRawChar",         (void*)FileWriteRawChar);
    ccAddExternalFunctionForPlugin("FileWriteRawLine",         (void*)FileWriteRawLine);
    ccAddExternalFunctionForPlugin("FindGUIID",                (void*)FindGUIID);
    ccAddExternalFunctionForPlugin("FlipScreen",               (void*)FlipScreen);
    ccAddExternalFunctionForPlugin("FloatToInt",               (void*)FloatToInt);
    ccAddExternalFunctionForPlugin("GetBackgroundFrame",       (void*)GetBackgroundFrame);
    ccAddExternalFunctionForPlugin("GetButtonPic",             (void*)GetButtonPic);
    ccAddExternalFunctionForPlugin("GetCharacterAt",           (void*)GetCharIDAtScreen);
    ccAddExternalFunctionForPlugin("GetCursorMode",            (void*)GetCursorMode);
    ccAddExternalFunctionForPlugin("GetDialogOption",          (void*)GetDialogOption);
    ccAddExternalFunctionForPlugin("GetGameOption",            (void*)GetGameOption);
    ccAddExternalFunctionForPlugin("GetGameParameter",         (void*)GetGameParameter);
    ccAddExternalFunctionForPlugin("GetGameSpeed",             (void*)GetGameSpeed);
    ccAddExternalFunctionForPlugin("GetGlobalInt",             (void*)GetGlobalInt);
    ccAddExternalFunctionForPlugin("GetGUIAt",                 (void*)GetGUIAt);
    ccAddExternalFunctionForPlugin("GetGUIObjectAt",           (void*)GetGUIObjectAt);
    ccAddExternalFunctionForPlugin("GetHotspotAt",             (void*)GetHotspotIDAtScreen);
    ccAddExternalFunctionForPlugin("GetHotspotName",           (void*)GetHotspotName);
    ccAddExternalFunctionForPlugin("GetHotspotPointX",         (void*)GetHotspotPointX);
    ccAddExternalFunctionForPlugin("GetHotspotPointY",         (void*)GetHotspotPointY);
    ccAddExternalFunctionForPlugin("GetHotspotProperty",       (void*)GetHotspotProperty);
    ccAddExternalFunctionForPlugin("GetHotspotPropertyText",   (void*)GetHotspotPropertyText);
    ccAddExternalFunctionForPlugin("GetInvAt",                 (void*)GetInvAt);
    ccAddExternalFunctionForPlugin("GetInvGraphic",            (void*)GetInvGraphic);
    ccAddExternalFunctionForPlugin("GetInvName",               (void*)GetInvName);
    ccAddExternalFunctionForPlugin("GetInvProperty",           (void*)GetInvProperty);
    ccAddExternalFunctionForPlugin("GetInvPropertyText",       (void*)GetInvPropertyText);
    ccAddExternalFunctionForPlugin("GetLocationType",          (void*)GetLocationType);
    ccAddExternalFunctionForPlugin("GetObjectAt",              (void*)GetObjectIDAtScreen);
    ccAddExternalFunctionForPlugin("GetObjectBaseline",        (void*)GetObjectBaseline);
    ccAddExternalFunctionForPlugin("GetObjectGraphic",         (void*)GetObjectGraphic);
    ccAddExternalFunctionForPlugin("GetObjectName",            (void*)GetObjectName);
    ccAddExternalFunctionForPlugin("GetObjectProperty",        (void*)GetObjectProperty);
    ccAddExternalFunctionForPlugin("GetObjectPropertyText",    (void*)GetObjectPropertyText);
    ccAddExternalFunctionForPlugin("GetObjectX",               (void*)GetObjectX);
    ccAddExternalFunctionForPlugin("GetObjectY",               (void*)GetObjectY);
    ccAddExternalFunctionForPlugin("GetPlayerCharacter",       (void*)GetPlayerCharacter);
    ccAddExternalFunctionForPlugin("GetRawTime",               (void*)GetRawTime);
    ccAddExternalFunctionForPlugin("GetRegionAt",              (void*)GetRegionIDAtRoom);
    ccAddExternalFunctionForPlugin("GetRoomProperty",          (void*)Room_GetProperty);
    ccAddExternalFunctionForPlugin("GetScalingAt",             (void*)GetScalingAt);
    ccAddExternalFunctionForPlugin("GetSliderValue",           (void*)GetSliderValue);
    ccAddExternalFunctionForPlugin("GetTextBoxText",           (void*)GetTextBoxText);
    ccAddExternalFunctionForPlugin("GetTextHeight",            (void*)GetTextHeight);
    ccAddExternalFunctionForPlugin("GetTextWidth",             (void*)GetTextWidth);
    ccAddExternalFunctionForPlugin("GetTime",                  (void*)sc_GetTime);
    ccAddExternalFunctionForPlugin("GetTranslation",           (void*)get_translation);
    ccAddExternalFunctionForPlugin("GetTranslationName",       (void*)GetTranslationName);
    ccAddExternalFunctionForPlugin("GetWalkableAreaAtRoom",    (void*)GetWalkableAreaAtRoom);
    ccAddExternalFunctionForPlugin("GetWalkableAreaAtScreen",  (void*)GetWalkableAreaAtScreen);
    ccAddExternalFunctionForPlugin("GiveScore",                (void*)GiveScore);
    ccAddExternalFunctionForPlugin("HasPlayerBeenInRoom",      (void*)HasPlayerBeenInRoom);
    ccAddExternalFunctionForPlugin("HideMouseCursor",          (void*)HideMouseCursor);
    ccAddExternalFunctionForPlugin("InputBox",                 (void*)sc_inputbox);
    ccAddExternalFunctionForPlugin("InterfaceOff",             (void*)InterfaceOff);
    ccAddExternalFunctionForPlugin("InterfaceOn",              (void*)InterfaceOn);
    ccAddExternalFunctionForPlugin("IntToFloat",               (void*)IntToFloat);
    ccAddExternalFunctionForPlugin("IsButtonDown",             (void*)IsButtonDown);
    ccAddExternalFunctionForPlugin("IsGamePaused",             (void*)IsGamePaused);
    ccAddExternalFunctionForPlugin("IsGUIOn",                  (void*)IsGUIOn);
    ccAddExternalFunctionForPlugin("IsInteractionAvailable",   (void*)IsInteractionAvailable);
    ccAddExternalFunctionForPlugin("IsInventoryInteractionAvailable", (void*)IsInventoryInteractionAvailable);
    ccAddExternalFunctionForPlugin("IsInterfaceEnabled",       (void*)IsInterfaceEnabled);
    ccAddExternalFunctionForPlugin("IsKeyPressed",             (void*)IsKeyPressed);
    ccAddExternalFunctionForPlugin("IsMusicVoxAvailable",      (void*)IsMusicVoxAvailable);
    ccAddExternalFunctionForPlugin("IsObjectAnimating",        (void*)IsObjectAnimating);
    ccAddExternalFunctionForPlugin("IsObjectMoving",           (void*)IsObjectMoving);
    ccAddExternalFunctionForPlugin("IsObjectOn",               (void*)IsObjectOn);
    ccAddExternalFunctionForPlugin("IsTimerExpired",           (void*)IsTimerExpired);
    ccAddExternalFunctionForPlugin("IsTranslationAvailable",   (void*)IsTranslationAvailable);
    ccAddExternalFunctionForPlugin("IsVoxAvailable",           (void*)IsVoxAvailable);
    ccAddExternalFunctionForPlugin("ListBoxAdd",               (void*)ListBoxAdd);
    ccAddExternalFunctionForPlugin("ListBoxClear",             (void*)ListBoxClear);
    ccAddExternalFunctionForPlugin("ListBoxDirList",           (void*)ListBoxDirList);
    ccAddExternalFunctionForPlugin("ListBoxGetItemText",       (void*)ListBoxGetItemText);
    ccAddExternalFunctionForPlugin("ListBoxGetNumItems",       (void*)ListBoxGetNumItems);
    ccAddExternalFunctionForPlugin("ListBoxGetSelected",       (void*)ListBoxGetSelected);
    ccAddExternalFunctionForPlugin("ListBoxRemove",            (void*)ListBoxRemove);
    ccAddExternalFunctionForPlugin("ListBoxSaveGameList",      (void*)ListBoxSaveGameList);
    ccAddExternalFunctionForPlugin("ListBoxSetSelected",       (void*)ListBoxSetSelected);
    ccAddExternalFunctionForPlugin("ListBoxSetTopItem",        (void*)ListBoxSetTopItem);
    ccAddExternalFunctionForPlugin("LoadImageFile",            (void*)LoadImageFile);
    ccAddExternalFunctionForPlugin("LoadSaveSlotScreenshot",   (void*)LoadSaveSlotScreenshot);
    ccAddExternalFunctionForPlugin("MergeObject",              (void*)MergeObject);
    ccAddExternalFunctionForPlugin("MoveCharacterToHotspot",   (void*)MoveCharacterToHotspot);
    ccAddExternalFunctionForPlugin("MoveObject",               (void*)MoveObject);
    ccAddExternalFunctionForPlugin("MoveObjectDirect",         (void*)MoveObjectDirect);
    ccAddExternalFunctionForPlugin("ObjectOff",                (void*)ObjectOff);
    ccAddExternalFunctionForPlugin("ObjectOn",                 (void*)ObjectOn);
    ccAddExternalFunctionForPlugin("PauseGame",                (void*)PauseGame);
    ccAddExternalFunctionForPlugin("PlayFlic",                 (void*)PlayFlic);
    ccAddExternalFunctionForPlugin("PlayVideo",                (void*)PlayVideo);
    ccAddExternalFunctionForPlugin("ProcessClick",             (void*)RoomProcessClick);
    ccAddExternalFunctionForPlugin("QuitGame",                 (void*)QuitGame);
    ccAddExternalFunctionForPlugin("Random",                   (void*)__Rand);
    ccAddExternalFunctionForPlugin("RefreshMouse",             (void*)RefreshMouse);
    ccAddExternalFunctionForPlugin("RemoveObjectTint",         (void*)RemoveObjectTint);
    ccAddExternalFunctionForPlugin("RemoveWalkableArea",       (void*)RemoveWalkableArea);
    ccAddExternalFunctionForPlugin("ResetRoom",                (void*)ResetRoom);
    ccAddExternalFunctionForPlugin("RestartGame",              (void*)restart_game);
    ccAddExternalFunctionForPlugin("RestoreGameDialog",        (void*)restore_game_dialog);
    ccAddExternalFunctionForPlugin("RestoreGameSlot",          (void*)RestoreGameSlot);
    ccAddExternalFunctionForPlugin("RestoreWalkableArea",      (void*)RestoreWalkableArea);
    ccAddExternalFunctionForPlugin("RunAGSGame",               (void*)RunAGSGame);
    ccAddExternalFunctionForPlugin("RunDialog",                (void*)RunDialog);
    ccAddExternalFunctionForPlugin("RunHotspotInteraction",    (void*)RunHotspotInteraction);
    ccAddExternalFunctionForPlugin("RunInventoryInteraction",  (void*)RunInventoryInteraction);
    ccAddExternalFunctionForPlugin("RunObjectInteraction",     (void*)RunObjectInteraction);
    ccAddExternalFunctionForPlugin("RunRegionInteraction",     (void*)RunRegionInteraction);
    ccAddExternalFunctionForPlugin("Said",                     (void*)Said);
    ccAddExternalFunctionForPlugin("SaveCursorForLocationChange", (void*)SaveCursorForLocationChange);
    ccAddExternalFunctionForPlugin("SaveGameDialog",           (void*)save_game_dialog);
    ccAddExternalFunctionForPlugin("SaveGameSlot",             (void*)save_game);
    ccAddExternalFunctionForPlugin("SaveScreenShot",           (void*)SaveScreenShot);
    ccAddExternalFunctionForPlugin("SetAmbientTint",           (void*)SetAmbientTint);
    ccAddExternalFunctionForPlugin("SetAreaLightLevel",        (void*)SetAreaLightLevel);
    ccAddExternalFunctionForPlugin("SetAreaScaling",           (void*)SetAreaScaling);
    ccAddExternalFunctionForPlugin("SetBackgroundFrame",       (void*)SetBackgroundFrame);
    ccAddExternalFunctionForPlugin("SetButtonPic",             (void*)SetButtonPic);
    ccAddExternalFunctionForPlugin("SetButtonText",            (void*)SetButtonText);
    ccAddExternalFunctionForPlugin("SetCursorMode",            (void*)set_cursor_mode);
    ccAddExternalFunctionForPlugin("SetDefaultCursor",         (void*)set_default_cursor);
    ccAddExternalFunctionForPlugin("SetDialogOption",          (void*)SetDialogOption);
    ccAddExternalFunctionForPlugin("SetFadeColor",             (void*)SetFadeColor);
    ccAddExternalFunctionForPlugin("SetGameOption",            (void*)SetGameOption);
    ccAddExternalFunctionForPlugin("SetGameSpeed",             (void*)SetGameSpeed);
    ccAddExternalFunctionForPlugin("SetGlobalInt",             (void*)SetGlobalInt);
    ccAddExternalFunctionForPlugin("SetGUIBackgroundPic",      (void*)SetGUIBackgroundPic);
    ccAddExternalFunctionForPlugin("SetGUIClickable",          (void*)SetGUIClickable);
    ccAddExternalFunctionForPlugin("SetGUIObjectEnabled",      (void*)SetGUIObjectEnabled);
    ccAddExternalFunctionForPlugin("SetGUIObjectPosition",     (void*)SetGUIObjectPosition);
    ccAddExternalFunctionForPlugin("SetGUIObjectSize",         (void*)SetGUIObjectSize);
    ccAddExternalFunctionForPlugin("SetGUIPosition",           (void*)SetGUIPosition);
    ccAddExternalFunctionForPlugin("SetGUISize",               (void*)SetGUISize);
    ccAddExternalFunctionForPlugin("SetGUITransparency",       (void*)SetGUITransparency);
    ccAddExternalFunctionForPlugin("SetGUIZOrder",             (void*)SetGUIZOrder);
    ccAddExternalFunctionForPlugin("SetInvItemName",           (void*)SetInvItemName);
    ccAddExternalFunctionForPlugin("SetInvItemPic",            (void*)set_inv_item_pic);
    ccAddExternalFunctionForPlugin("SetLabelColor",            (void*)SetLabelColor);
    ccAddExternalFunctionForPlugin("SetLabelFont",             (void*)SetLabelFont);
    ccAddExternalFunctionForPlugin("SetLabelText",             (void*)SetLabelText);
    ccAddExternalFunctionForPlugin("SetMouseBounds",           (void*)SetMouseBounds);
    ccAddExternalFunctionForPlugin("SetMouseCursor",           (void*)set_mouse_cursor);
    ccAddExternalFunctionForPlugin("SetMousePosition",         (void*)SetMousePosition);
    ccAddExternalFunctionForPlugin("SetMultitaskingMode",      (void*)SetMultitasking);
    ccAddExternalFunctionForPlugin("SetNextCursorMode",        (void*)SetNextCursor);
    ccAddExternalFunctionForPlugin("SetNextScreenTransition",  (void*)SetNextScreenTransition);
    ccAddExternalFunctionForPlugin("SetNormalFont",            (void*)SetNormalFont);
    ccAddExternalFunctionForPlugin("SetObjectBaseline",        (void*)SetObjectBaseline);
    ccAddExternalFunctionForPlugin("SetObjectClickable",       (void*)SetObjectClickable);
    ccAddExternalFunctionForPlugin("SetObjectFrame",           (void*)SetObjectFrame);
    ccAddExternalFunctionForPlugin("SetObjectGraphic",         (void*)SetObjectGraphic);
    ccAddExternalFunctionForPlugin("SetObjectIgnoreWalkbehinds", (void*)SetObjectIgnoreWalkbehinds);
    ccAddExternalFunctionForPlugin("SetObjectPosition",        (void*)SetObjectPosition);
    ccAddExternalFunctionForPlugin("SetObjectTint",            (void*)SetObjectTint);
    ccAddExternalFunctionForPlugin("SetObjectTransparency",    (void*)SetObjectTransparency);
    ccAddExternalFunctionForPlugin("SetObjectView",            (void*)SetObjectView);
    ccAddExternalFunctionForPlugin("SetPalRGB",                (void*)SetPalRGB);
    ccAddExternalFunctionForPlugin("SetRegionTint",            (void*)SetRegionTint);
    ccAddExternalFunctionForPlugin("SetRestartPoint",          (void*)SetRestartPoint);
    ccAddExternalFunctionForPlugin("SetScreenTransition",      (void*)SetScreenTransition);
    ccAddExternalFunctionForPlugin("SetSliderValue",           (void*)SetSliderValue);
    ccAddExternalFunctionForPlugin("SetSpeechFont",            (void*)SetSpeechFont);
    ccAddExternalFunctionForPlugin("SetSpeechVolume",          (void*)SetSpeechVolume);
    ccAddExternalFunctionForPlugin("SetTextBoxFont",           (void*)SetTextBoxFont);
    ccAddExternalFunctionForPlugin("SetTextBoxText",           (void*)SetTextBoxText);
    ccAddExternalFunctionForPlugin("SetTextWindowGUI",         (void*)SetTextWindowGUI);
    ccAddExternalFunctionForPlugin("SetTimer",                 (void*)script_SetTimer);
    ccAddExternalFunctionForPlugin("SetWalkBehindBase",        (void*)SetWalkBehindBase);
    ccAddExternalFunctionForPlugin("ShakeScreen",              (void*)ShakeScreen);
    ccAddExternalFunctionForPlugin("ShakeScreenBackground",    (void*)ShakeScreenBackground);
    ccAddExternalFunctionForPlugin("ShowMouseCursor",          (void*)ShowMouseCursor);
    ccAddExternalFunctionForPlugin("SkipUntilCharacterStops",  (void*)SkipUntilCharacterStops);
    ccAddExternalFunctionForPlugin("StartCutscene",            (void*)StartCutscene);
    ccAddExternalFunctionForPlugin("StartRecording",           (void*)scStartRecording);
    ccAddExternalFunctionForPlugin("StopChannel",              (void*)stop_and_destroy_channel);
    ccAddExternalFunctionForPlugin("StopDialog",               (void*)StopDialog);
    ccAddExternalFunctionForPlugin("StopMoving",               (void*)StopMoving);
    ccAddExternalFunctionForPlugin("StopObjectMoving",         (void*)StopObjectMoving);
    ccAddExternalFunctionForPlugin("TintScreen",               (void*)TintScreen);
    ccAddExternalFunctionForPlugin("UnPauseGame",              (void*)UnPauseGame);
    ccAddExternalFunctionForPlugin("UpdateInventory",          (void*)update_invorder);
    ccAddExternalFunctionForPlugin("UpdatePalette",            (void*)UpdatePalette);
    ccAddExternalFunctionForPlugin("Wait",                     (void*)scrWait);
    ccAddExternalFunctionForPlugin("WaitKey",                  (void*)WaitKey);
    ccAddExternalFunctionForPlugin("WaitMouseKey",             (void*)WaitMouseKey);
    ccAddExternalFunctionForPlugin("WaitInput",                (void*)WaitInput);
}
