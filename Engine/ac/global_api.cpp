//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
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
#include "ac/global_character.h"
#include "ac/global_debug.h"
#include "ac/global_dialog.h"
#include "ac/global_display.h"
#include "ac/global_dynamicsprite.h"
#include "ac/global_file.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_hotspot.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_object.h"
#include "ac/global_palette.h"
#include "ac/global_region.h"
#include "ac/global_room.h"
#include "ac/global_screen.h"
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

// void (char*texx, ...)
RuntimeScriptValue Sc_sc_AbortGame(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(_sc_AbortGame, 1);
    _sc_AbortGame(scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
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
    API_SCALL_SCRIPT_SPRINTF(DisplayAtY, 2);
    DisplayAtY(params[0].IValue, scsf_buffer);
    return RuntimeScriptValue((int32_t)0);
}

// void (int ypos, int ttexcol, int backcol, char *title, char*texx, ...)
RuntimeScriptValue Sc_DisplayTopBar(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_SCRIPT_SPRINTF(DisplayTopBar, 5);
    DisplayTopBar(params[0].IValue, params[1].IValue, params[2].IValue, params[3].CStr, scsf_buffer);
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
RuntimeScriptValue Sc_FadeOut(const RuntimeScriptValue *params, int32_t param_count)
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

// int ()
RuntimeScriptValue Sc_GetGameSpeed(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetGameSpeed);
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
    API_SCALL_OBJAUTO(ScriptDrawingSurface, GetDrawingSurfaceForWalkableArea);
}

RuntimeScriptValue Sc_GetDrawingSurfaceForWalkbehind(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDrawingSurface, GetDrawingSurfaceForWalkbehind);
}

// int (int roomnum)
RuntimeScriptValue Sc_HasPlayerBeenInRoom(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(HasPlayerBeenInRoom);
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
RuntimeScriptValue Sc_PlayFlic(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(PlayFlic);
}

// void (const char* name, int skip, int flags)
RuntimeScriptValue Sc_PlayVideo(const RuntimeScriptValue *params, int32_t param_count)
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

extern RuntimeScriptValue Sc_SetSpeechFont(const RuntimeScriptValue *params, int32_t param_count);

// void (int newvol)
RuntimeScriptValue Sc_SetSpeechVolume(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSpeechVolume);
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
// Exclusive variadic API implementation for Plugins
//
//=============================================================================

// void (char*texx, ...)
void ScPl_sc_AbortGame(const char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    _sc_AbortGame(scsf_buffer);
}

void ScPl_Display(char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    DisplaySimple(scsf_buffer);
}

void ScPl_DisplayAt(int xxp, int yyp, int widd, char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    DisplayAt(xxp, yyp, widd, scsf_buffer);
}

void ScPl_DisplayAtY(int ypos, char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    DisplayAtY(ypos, scsf_buffer);
}

// void (int ypos, int ttexcol, int backcol, char *title, char*texx, ...)
void ScPl_DisplayTopBar(int ypos, int ttexcol, int backcol, char *title, char *texx, ...)
{
    API_PLUGIN_SCRIPT_SPRINTF(texx);
    DisplayTopBar(ypos, ttexcol, backcol, title, scsf_buffer);
}


void RegisterGlobalAPI()
{
    ScFnRegister global_api[] = {
        { "AbortGame",                Sc_sc_AbortGame, ScPl_sc_AbortGame },
        { "AreThingsOverlapping",     API_FN_PAIR(AreThingsOverlapping) },
        { "CallRoomScript",           API_FN_PAIR(CallRoomScript) },
        { "ChangeCursorGraphic",      API_FN_PAIR(ChangeCursorGraphic) },
        { "ChangeCursorHotspot",      API_FN_PAIR(ChangeCursorHotspot) },
        { "ClaimEvent",               API_FN_PAIR(ClaimEvent) },
        { "CyclePalette",             API_FN_PAIR(CyclePalette) },
        { "Debug",                    API_FN_PAIR(script_debug) },
        { "DeleteSaveSlot",           API_FN_PAIR(DeleteSaveSlot) },
        { "DeleteSprite",             API_FN_PAIR(free_dynamic_sprite) },
        { "DisableCursorMode",        API_FN_PAIR(disable_cursor_mode) },
        { "DisableGroundLevelAreas",  API_FN_PAIR(DisableGroundLevelAreas) },
        { "DisableHotspot",           API_FN_PAIR(DisableHotspot) },
        { "DisableInterface",         API_FN_PAIR(DisableInterface) },
        { "DisableRegion",            API_FN_PAIR(DisableRegion) },
        { "Display",                  Sc_Display, ScPl_Display },
        { "DisplayAt",                Sc_DisplayAt, ScPl_DisplayAt },
        // CHECKME: this function was non-variadic prior to 3.6.1, but AGS compiler does
        // not produce "name^argnum" symbol id for non-member functions for some reason :/
        // do we have to do anything about this here? like, test vs script API version...
        { "DisplayAtY",               Sc_DisplayAtY, ScPl_DisplayAtY },
        { "DisplayTopBar",            Sc_DisplayTopBar, ScPl_DisplayTopBar },
        { "EnableCursorMode",         API_FN_PAIR(enable_cursor_mode) },
        { "EnableGroundLevelAreas",   API_FN_PAIR(EnableGroundLevelAreas) },
        { "EnableHotspot",            API_FN_PAIR(EnableHotspot) },
        { "EnableInterface",          API_FN_PAIR(EnableInterface) },
        { "EnableRegion",             API_FN_PAIR(EnableRegion) },
        { "EndCutscene",              API_FN_PAIR(EndCutscene) },
        { "FadeIn",                   API_FN_PAIR(FadeIn) },
        { "FadeOut",                  API_FN_PAIR(FadeOut) },
        { "FileClose",                API_FN_PAIR(FileClose) },
        { "FileIsEOF",                API_FN_PAIR(FileIsEOF) },
        { "FileIsError",              API_FN_PAIR(FileIsError) },
        // NOTE: FileOpenCMode is a backwards-compatible replacement for old-style global script function FileOpen
        { "FileOpen",                 API_FN_PAIR(FileOpenCMode) },
        { "FileRead",                 API_FN_PAIR(FileRead) },
        { "FileReadInt",              API_FN_PAIR(FileReadInt) },
        { "FileReadRawChar",          API_FN_PAIR(FileReadRawChar) },
        { "FileReadRawInt",           API_FN_PAIR(FileReadRawInt) },
        { "FileWrite",                API_FN_PAIR(FileWrite) },
        { "FileWriteInt",             API_FN_PAIR(FileWriteInt) },
        { "FileWriteRawChar",         API_FN_PAIR(FileWriteRawChar) },
        { "FileWriteRawLine",         API_FN_PAIR(FileWriteRawLine) },
        { "FindGUIID",                API_FN_PAIR(FindGUIID) },
        { "FlipScreen",               API_FN_PAIR(FlipScreen) },
        { "FloatToInt",               API_FN_PAIR(FloatToInt) },
        { "GetBackgroundFrame",       API_FN_PAIR(GetBackgroundFrame) },
        { "GetCharacterAt",           API_FN_PAIR(GetCharIDAtScreen) },
        { "GetCursorMode",            API_FN_PAIR(GetCursorMode) },
        { "GetDialogOption",          API_FN_PAIR(GetDialogOption) },
        { "GetGameOption",            API_FN_PAIR(GetGameOption) },
        { "GetGameSpeed",             API_FN_PAIR(GetGameSpeed) },
        { "GetHotspotAt",             API_FN_PAIR(GetHotspotIDAtScreen) },
        { "GetHotspotName",           API_FN_PAIR(GetHotspotName) },
        { "GetHotspotPointX",         API_FN_PAIR(GetHotspotPointX) },
        { "GetHotspotPointY",         API_FN_PAIR(GetHotspotPointY) },
        { "GetHotspotProperty",       API_FN_PAIR(GetHotspotProperty) },
        { "GetHotspotPropertyText",   API_FN_PAIR(GetHotspotPropertyText) },
        { "GetInvAt",                 API_FN_PAIR(GetInvAt) },
        { "GetInvGraphic",            API_FN_PAIR(GetInvGraphic) },
        { "GetInvName",               API_FN_PAIR(GetInvName) },
        { "GetInvProperty",           API_FN_PAIR(GetInvProperty) },
        { "GetInvPropertyText",       API_FN_PAIR(GetInvPropertyText) },
        { "GetLocationType",          API_FN_PAIR(GetLocationType) },
        { "GetObjectAt",              API_FN_PAIR(GetObjectIDAtScreen) },
        { "GetObjectBaseline",        API_FN_PAIR(GetObjectBaseline) },
        { "GetObjectGraphic",         API_FN_PAIR(GetObjectGraphic) },
        { "GetObjectName",            API_FN_PAIR(GetObjectName) },
        { "GetObjectProperty",        API_FN_PAIR(GetObjectProperty) },
        { "GetObjectPropertyText",    API_FN_PAIR(GetObjectPropertyText) },
        { "GetObjectX",               API_FN_PAIR(GetObjectX) },
        { "GetObjectY",               API_FN_PAIR(GetObjectY) },
        { "GetPlayerCharacter",       API_FN_PAIR(GetPlayerCharacter) },
        { "GetRegionAt",              API_FN_PAIR(GetRegionIDAtRoom) },
        { "GetRoomProperty",          API_FN_PAIR(Room_GetProperty) },
        { "GetScalingAt",             API_FN_PAIR(GetScalingAt) },
        { "GetTextHeight",            API_FN_PAIR(GetTextHeight) },
        { "GetTextWidth",             API_FN_PAIR(GetTextWidth) },
        { "GetFontHeight",            API_FN_PAIR(GetFontHeight) },
        { "GetFontLineSpacing",       API_FN_PAIR(GetFontLineSpacing) },
        { "GetTranslation",           API_FN_PAIR(get_translation) },
        { "GetTranslationName",       API_FN_PAIR(GetTranslationName) },
        { "GetWalkableAreaAtRoom",    API_FN_PAIR(GetWalkableAreaAtRoom) },
        { "GetWalkableAreaAt",        API_FN_PAIR(GetWalkableAreaAtScreen) },
        { "GetWalkableAreaAtScreen",  API_FN_PAIR(GetWalkableAreaAtScreen) },
        { "GetDrawingSurfaceForWalkableArea", API_FN_PAIR(GetDrawingSurfaceForWalkableArea) },
        { "GetDrawingSurfaceForWalkbehind", API_FN_PAIR(GetDrawingSurfaceForWalkbehind) },
        { "HasPlayerBeenInRoom",      API_FN_PAIR(HasPlayerBeenInRoom) },
        { "InputBox",                 API_FN_PAIR(sc_inputbox) },
        { "InterfaceOff",             API_FN_PAIR(InterfaceOff) },
        { "InterfaceOn",              API_FN_PAIR(InterfaceOn) },
        { "IntToFloat",               API_FN_PAIR(IntToFloat) },
        { "IsButtonDown",             API_FN_PAIR(IsButtonDown) },
        { "IsGamePaused",             API_FN_PAIR(IsGamePaused) },
        { "IsInteractionAvailable",   API_FN_PAIR(IsInteractionAvailable) },
        { "IsInventoryInteractionAvailable", API_FN_PAIR(IsInventoryInteractionAvailable) },
        { "IsInterfaceEnabled",       API_FN_PAIR(IsInterfaceEnabled) },
        { "IsKeyPressed",             API_FN_PAIR(IsKeyPressed) },
        { "IsMusicVoxAvailable",      API_FN_PAIR(IsMusicVoxAvailable) },
        { "IsObjectAnimating",        API_FN_PAIR(IsObjectAnimating) },
        { "IsObjectMoving",           API_FN_PAIR(IsObjectMoving) },
        { "IsObjectOn",               API_FN_PAIR(IsObjectOn) },
        { "IsTimerExpired",           API_FN_PAIR(IsTimerExpired) },
        { "IsTranslationAvailable",   API_FN_PAIR(IsTranslationAvailable) },
        { "IsVoxAvailable",           API_FN_PAIR(IsVoxAvailable) },
        { "LoadImageFile",            API_FN_PAIR(LoadImageFile) },
        { "LoadSaveSlotScreenshot",   API_FN_PAIR(LoadSaveSlotScreenshot) },
        { "MergeObject",              API_FN_PAIR(MergeObject) },
        { "MoveCharacterToHotspot",   API_FN_PAIR(MoveCharacterToHotspot) },
        { "MoveObject",               API_FN_PAIR(MoveObject) },
        { "MoveObjectDirect",         API_FN_PAIR(MoveObjectDirect) },
        { "ObjectOff",                API_FN_PAIR(ObjectOff) },
        { "ObjectOn",                 API_FN_PAIR(ObjectOn) },
        { "PauseGame",                API_FN_PAIR(PauseGame) },
        { "PlayFlic",                 API_FN_PAIR(PlayFlic) },
        { "PlayVideo",                API_FN_PAIR(PlayVideo) },
        { "QuitGame",                 API_FN_PAIR(QuitGame) },
        { "Random",                   Sc_Rand, __Rand },
        { "RefreshMouse",             API_FN_PAIR(RefreshMouse) },
        { "RemoveObjectTint",         API_FN_PAIR(RemoveObjectTint) },
        { "RemoveWalkableArea",       API_FN_PAIR(RemoveWalkableArea) },
        { "ResetRoom",                API_FN_PAIR(ResetRoom) },
        { "RestartGame",              API_FN_PAIR(restart_game) },
        { "RestoreGameDialog",        API_FN_PAIR(restore_game_dialog) },
        { "RestoreGameSlot",          API_FN_PAIR(RestoreGameSlot) },
        { "RestoreWalkableArea",      API_FN_PAIR(RestoreWalkableArea) },
        { "RunAGSGame",               API_FN_PAIR(RunAGSGame) },
        { "RunDialog",                API_FN_PAIR(RunDialog) },
        { "RunHotspotInteraction",    API_FN_PAIR(RunHotspotInteraction) },
        { "RunInventoryInteraction",  API_FN_PAIR(RunInventoryInteraction) },
        { "RunObjectInteraction",     API_FN_PAIR(RunObjectInteraction) },
        { "RunRegionInteraction",     API_FN_PAIR(RunRegionInteraction) },
        { "Said",                     API_FN_PAIR(Said) },
        { "SaveCursorForLocationChange", API_FN_PAIR(SaveCursorForLocationChange) },
        { "SaveGameDialog",           API_FN_PAIR(save_game_dialog) },
        { "SaveGameSlot",             API_FN_PAIR(save_game) },
        { "SaveScreenShot",           API_FN_PAIR(SaveScreenShot) },
        { "SetAmbientTint",           API_FN_PAIR(SetAmbientTint) },
        { "SetAmbientLightLevel",     API_FN_PAIR(SetAmbientLightLevel) },
        { "SetAreaLightLevel",        API_FN_PAIR(SetAreaLightLevel) },
        { "SetAreaScaling",           API_FN_PAIR(SetAreaScaling) },
        { "SetBackgroundFrame",       API_FN_PAIR(SetBackgroundFrame) },
        { "SetCursorMode",            API_FN_PAIR(set_cursor_mode) },
        { "SetDefaultCursor",         API_FN_PAIR(set_default_cursor) },
        { "SetDialogOption",          API_FN_PAIR(SetDialogOption) },
        { "SetFadeColor",             API_FN_PAIR(SetFadeColor) },
        { "SetGameOption",            API_FN_PAIR(SetGameOption) },
        { "SetGameSpeed",             API_FN_PAIR(SetGameSpeed) },
        { "SetInvItemName",           API_FN_PAIR(SetInvItemName) },
        { "SetInvItemPic",            API_FN_PAIR(set_inv_item_pic) },
        { "SetMouseBounds",           API_FN_PAIR(SetMouseBounds) },
        { "SetMouseCursor",           API_FN_PAIR(set_mouse_cursor) },
        { "SetMousePosition",         API_FN_PAIR(SetMousePosition) },
        { "SetMultitaskingMode",      API_FN_PAIR(SetMultitasking) },
        { "SetNextCursorMode",        API_FN_PAIR(SetNextCursor) },
        { "SetNextScreenTransition",  API_FN_PAIR(SetNextScreenTransition) },
        { "SetNormalFont",            API_FN_PAIR(SetNormalFont) },
        { "SetObjectBaseline",        API_FN_PAIR(SetObjectBaseline) },
        { "SetObjectClickable",       API_FN_PAIR(SetObjectClickable) },
        { "SetObjectFrame",           API_FN_PAIR(SetObjectFrame) },
        { "SetObjectGraphic",         API_FN_PAIR(SetObjectGraphic) },
        { "SetObjectIgnoreWalkbehinds", API_FN_PAIR(SetObjectIgnoreWalkbehinds) },
        { "SetObjectPosition",        API_FN_PAIR(SetObjectPosition) },
        { "SetObjectTint",            API_FN_PAIR(SetObjectTint) },
        { "SetObjectTransparency",    API_FN_PAIR(SetObjectTransparency) },
        { "SetObjectView",            API_FN_PAIR(SetObjectView) },
        { "SetPalRGB",                API_FN_PAIR(SetPalRGB) },
        { "SetRegionTint",            API_FN_PAIR(SetRegionTint) },
        { "SetRestartPoint",          API_FN_PAIR(SetRestartPoint) },
        { "SetScreenTransition",      API_FN_PAIR(SetScreenTransition) },
        { "SetSpeechFont",            API_FN_PAIR(SetSpeechFont) },
        { "SetSpeechVolume",          API_FN_PAIR(SetSpeechVolume) },
        { "SetTextWindowGUI",         API_FN_PAIR(SetTextWindowGUI) },
        { "SetTimer",                 API_FN_PAIR(script_SetTimer) },
        { "SetWalkBehindBase",        API_FN_PAIR(SetWalkBehindBase) },
        { "ShakeScreen",              API_FN_PAIR(ShakeScreen) },
        { "ShakeScreenBackground",    API_FN_PAIR(ShakeScreenBackground) },
        { "SkipCutscene",             API_FN_PAIR(SkipCutscene) },
        { "SkipUntilCharacterStops",  API_FN_PAIR(SkipUntilCharacterStops) },
        { "StartCutscene",            API_FN_PAIR(StartCutscene) },
        { "StopChannel",              API_FN_PAIR(stop_and_destroy_channel) },
        { "StopDialog",               API_FN_PAIR(StopDialog) },
        { "StopMoving",               API_FN_PAIR(StopMoving) },
        { "StopObjectMoving",         API_FN_PAIR(StopObjectMoving) },
        { "StringToInt",              API_FN_PAIR(StringToInt) },
        { "StrLen",                   API_FN_PAIR(strlen) },
        { "TintScreen",               API_FN_PAIR(TintScreen) },
        { "UnPauseGame",              API_FN_PAIR(UnPauseGame) },
        { "UpdateInventory",          API_FN_PAIR(update_invorder) },
        { "UpdatePalette",            API_FN_PAIR(UpdatePalette) },
        { "Wait",                     API_FN_PAIR(scrWait) },
        { "WaitKey",                  API_FN_PAIR(WaitKey) },
        { "WaitMouse",                API_FN_PAIR(WaitMouse) },
        { "WaitMouseKey",             API_FN_PAIR(WaitMouseKey) },
        { "WaitInput",                API_FN_PAIR(WaitInput) },
        { "SkipWait",                 API_FN_PAIR(SkipWait) },
    };

    ccAddExternalFunctions(global_api);
}
