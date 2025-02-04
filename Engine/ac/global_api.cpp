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
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

#include "ac/character.h"
#include "ac/display.h"
#include "ac/dynamicsprite.h"
#include "ac/event.h"
#include "ac/game.h"
#include "ac/gamestructdefines.h"
#include "ac/global_audio.h"
#include "ac/global_debug.h"
#include "ac/global_display.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_palette.h"
#include "ac/global_room.h"
#include "ac/global_screen.h"
#include "ac/global_timer.h"
#include "ac/global_translation.h"
#include "ac/global_video.h"
#include "ac/global_walkablearea.h"
#include "ac/global_walkbehind.h"
#include "ac/math.h"
#include "ac/mouse.h"
#include "ac/object.h"
#include "ac/parser.h"
#include "ac/region.h"
#include "ac/string.h"
#include "ac/room.h"
#include "ac/dynobj/scriptstring.h"
#include "media/video/video.h"
#include "media/audio/audio_system.h"
#include "util/string_compat.h"

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

RuntimeScriptValue Sc_CopySaveSlot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(CopySaveSlot);
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

// void ()
RuntimeScriptValue Sc_DisableInterface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(DisableInterface);
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

// void ()
RuntimeScriptValue Sc_EnableInterface(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(EnableInterface);
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

extern RuntimeScriptValue Sc_GetCursorMode(const RuntimeScriptValue *params, int32_t param_count);

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
RuntimeScriptValue Sc_GetLocationType(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetLocationType);
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

RuntimeScriptValue Sc_GetTimerPos(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetTimerPos);
}

// char * (const char *text)
RuntimeScriptValue Sc_get_translation(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(char, myScriptStringImpl, get_translation, const char);
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
RuntimeScriptValue Sc_ShowInputBox(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(ShowInputBox, const char, char);
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

RuntimeScriptValue Sc_MoveSaveSlot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(MoveSaveSlot);
}

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

RuntimeScriptValue Sc_restore_game_dialog2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(restore_game_dialog2);
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

extern RuntimeScriptValue Sc_Said(const RuntimeScriptValue *params, int32_t param_count);

extern RuntimeScriptValue Sc_SaveCursorForLocationChange(const RuntimeScriptValue *params, int32_t param_count);

// void ()
RuntimeScriptValue Sc_save_game_dialog(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(save_game_dialog);
}

RuntimeScriptValue Sc_save_game_dialog2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(save_game_dialog2);
}

RuntimeScriptValue Sc_SaveGameSlot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ_PINT(SaveGameSlot, const char);
}

RuntimeScriptValue Sc_SaveGameSlot2(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(SaveGameSlot2, const char);
}

// int (char*namm)
RuntimeScriptValue Sc_SaveScreenShot(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(SaveScreenShot, const char);
}

RuntimeScriptValue Sc_SendEvent(const RuntimeScriptValue *params, int32_t param_count)
{
    ASSERT_PARAM_COUNT(run_on_event, 5); \
    run_on_event(static_cast<AGSScriptEventType>(params[0].IValue), params[1].IValue, params[2].IValue, params[3].IValue, params[4].IValue);
    return RuntimeScriptValue(0);
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

// void (int inndx,int rr,int gg,int bb)
RuntimeScriptValue Sc_SetPalRGB(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetPalRGB);
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
RuntimeScriptValue Sc_SetTimer(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetTimer);
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
RuntimeScriptValue Sc_UpdateInventory(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(UpdateInventory);
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


void RegisterGlobalAPI(ScriptAPIVersion base_api, ScriptAPIVersion /*compat_api*/)
{
    ScFnRegister global_api[] = {
        { "AbortGame",                Sc_sc_AbortGame, ScPl_sc_AbortGame },
        { "AreThingsOverlapping",     API_FN_PAIR(AreThingsOverlapping) },
        { "CallRoomScript",           API_FN_PAIR(CallRoomScript) },
        { "ChangeCursorGraphic",      API_FN_PAIR(ChangeCursorGraphic) },
        { "ChangeCursorHotspot",      API_FN_PAIR(ChangeCursorHotspot) },
        { "ClaimEvent",               API_FN_PAIR(ClaimEvent) },
        { "CopySaveSlot",             API_FN_PAIR(CopySaveSlot) },
        { "CyclePalette",             API_FN_PAIR(CyclePalette) },
        { "Debug",                    API_FN_PAIR(script_debug) },
        { "DeleteSaveSlot",           API_FN_PAIR(DeleteSaveSlot) },
        { "DeleteSprite",             API_FN_PAIR(free_dynamic_sprite) },
        { "DisableCursorMode",        API_FN_PAIR(disable_cursor_mode) },
        { "DisableGroundLevelAreas",  API_FN_PAIR(DisableGroundLevelAreas) },
        { "DisableInterface",         API_FN_PAIR(DisableInterface) },
        { "Display",                  Sc_Display, ScPl_Display },
        { "DisplayAt",                Sc_DisplayAt, ScPl_DisplayAt },
        // CHECKME: this function was non-variadic prior to 3.6.1, but AGS compiler does
        // not produce "name^argnum" symbol id for non-member functions for some reason :/
        // do we have to do anything about this here? like, test vs script API version...
        { "DisplayAtY",               Sc_DisplayAtY, ScPl_DisplayAtY },
        { "DisplayTopBar",            Sc_DisplayTopBar, ScPl_DisplayTopBar },
        { "EnableCursorMode",         API_FN_PAIR(enable_cursor_mode) },
        { "EnableGroundLevelAreas",   API_FN_PAIR(EnableGroundLevelAreas) },
        { "EnableInterface",          API_FN_PAIR(EnableInterface) },
        { "EndCutscene",              API_FN_PAIR(EndCutscene) },
        { "FadeIn",                   API_FN_PAIR(FadeIn) },
        { "FadeOut",                  API_FN_PAIR(FadeOut) },
        { "FindGUIID",                API_FN_PAIR(FindGUIID) },
        { "FlipScreen",               API_FN_PAIR(FlipScreen) },
        { "FloatToInt",               API_FN_PAIR(FloatToInt) },
        { "GetBackgroundFrame",       API_FN_PAIR(GetBackgroundFrame) },
        { "GetCursorMode",            API_FN_PAIR(GetCursorMode) },
        { "GetGameOption",            API_FN_PAIR(GetGameOption) },
        { "GetGameSpeed",             API_FN_PAIR(GetGameSpeed) },
        { "GetLocationType",          API_FN_PAIR(GetLocationType) },
        { "GetRegionAt",              API_FN_PAIR(GetRegionIDAtRoom) },
        { "GetRoomProperty",          API_FN_PAIR(Room_GetProperty) },
        { "GetScalingAt",             API_FN_PAIR(GetScalingAt) },
        { "GetTextHeight",            API_FN_PAIR(GetTextHeight) },
        { "GetTextWidth",             API_FN_PAIR(GetTextWidth) },
        { "GetFontHeight",            API_FN_PAIR(GetFontHeight) },
        { "GetFontLineSpacing",       API_FN_PAIR(GetFontLineSpacing) },
        { "GetTimerPos",              API_FN_PAIR(GetTimerPos) },
        { "GetTranslation",           API_FN_PAIR(get_translation) },
        { "GetWalkableAreaAtRoom",    API_FN_PAIR(GetWalkableAreaAtRoom) },
        { "GetWalkableAreaAt",        API_FN_PAIR(GetWalkableAreaAtScreen) },
        { "GetWalkableAreaAtScreen",  API_FN_PAIR(GetWalkableAreaAtScreen) },
        { "GetDrawingSurfaceForWalkableArea", API_FN_PAIR(GetDrawingSurfaceForWalkableArea) },
        { "GetDrawingSurfaceForWalkbehind", API_FN_PAIR(GetDrawingSurfaceForWalkbehind) },
        { "HasPlayerBeenInRoom",      API_FN_PAIR(HasPlayerBeenInRoom) },
        { "InputBox",                 API_FN_PAIR(ShowInputBox) },
        { "InterfaceOff",             API_FN_PAIR(InterfaceOff) },
        { "InterfaceOn",              API_FN_PAIR(InterfaceOn) },
        { "IntToFloat",               API_FN_PAIR(IntToFloat) },
        { "IsButtonDown",             API_FN_PAIR(IsButtonDown) },
        { "IsGamePaused",             API_FN_PAIR(IsGamePaused) },
        { "IsInteractionAvailable",   API_FN_PAIR(IsInteractionAvailable) },
        { "IsInterfaceEnabled",       API_FN_PAIR(IsInterfaceEnabled) },
        { "IsKeyPressed",             API_FN_PAIR(IsKeyPressed) },
        { "IsMusicVoxAvailable",      API_FN_PAIR(IsMusicVoxAvailable) },
        { "IsTimerExpired",           API_FN_PAIR(IsTimerExpired) },
        { "IsTranslationAvailable",   API_FN_PAIR(IsTranslationAvailable) },
        { "IsVoxAvailable",           API_FN_PAIR(IsVoxAvailable) },
        { "MoveSaveSlot",             API_FN_PAIR(MoveSaveSlot) },
        { "MoveSaveSlot",             API_FN_PAIR(MoveSaveSlot) },
        { "PauseGame",                API_FN_PAIR(PauseGame) },
        { "PlayFlic",                 API_FN_PAIR(PlayFlic) },
        { "PlayVideo",                API_FN_PAIR(PlayVideo) },
        { "QuitGame",                 API_FN_PAIR(QuitGame) },
        { "Random",                   Sc_Rand, __Rand },
        { "RefreshMouse",             API_FN_PAIR(RefreshMouse) },
        { "RemoveWalkableArea",       API_FN_PAIR(RemoveWalkableArea) },
        { "ResetRoom",                API_FN_PAIR(ResetRoom) },
        { "RestartGame",              API_FN_PAIR(restart_game) },
        { "RestoreGameSlot",          API_FN_PAIR(RestoreGameSlot) },
        { "RestoreWalkableArea",      API_FN_PAIR(RestoreWalkableArea) },
        { "RunAGSGame",               API_FN_PAIR(RunAGSGame) },
        { "Said",                     API_FN_PAIR(Said) },
        { "SaveCursorForLocationChange", API_FN_PAIR(SaveCursorForLocationChange) },
        { "SaveScreenShot",           API_FN_PAIR(SaveScreenShot) },
        { "SendEvent",                Sc_SendEvent, run_on_event },
        { "SetAmbientTint",           API_FN_PAIR(SetAmbientTint) },
        { "SetAmbientLightLevel",     API_FN_PAIR(SetAmbientLightLevel) },
        { "SetAreaScaling",           API_FN_PAIR(SetAreaScaling) },
        { "SetBackgroundFrame",       API_FN_PAIR(SetBackgroundFrame) },
        { "SetCursorMode",            API_FN_PAIR(set_cursor_mode) },
        { "SetDefaultCursor",         API_FN_PAIR(set_default_cursor) },
        { "SetFadeColor",             API_FN_PAIR(SetFadeColor) },
        { "SetGameOption",            API_FN_PAIR(SetGameOption) },
        { "SetGameSpeed",             API_FN_PAIR(SetGameSpeed) },
        { "SetMouseBounds",           API_FN_PAIR(SetMouseBounds) },
        { "SetMouseCursor",           API_FN_PAIR(set_mouse_cursor) },
        { "SetMousePosition",         API_FN_PAIR(SetMousePosition) },
        { "SetMultitaskingMode",      API_FN_PAIR(SetMultitasking) },
        { "SetNextCursorMode",        API_FN_PAIR(SetNextCursor) },
        { "SetNextScreenTransition",  API_FN_PAIR(SetNextScreenTransition) },
        { "SetPalRGB",                API_FN_PAIR(SetPalRGB) },
        { "SetRestartPoint",          API_FN_PAIR(SetRestartPoint) },
        { "SetScreenTransition",      API_FN_PAIR(SetScreenTransition) },
        { "SetSpeechVolume",          API_FN_PAIR(SetSpeechVolume) },
        { "SetTextWindowGUI",         API_FN_PAIR(SetTextWindowGUI) },
        { "SetTimer",                 API_FN_PAIR(SetTimer) },
        { "SetWalkBehindBase",        API_FN_PAIR(SetWalkBehindBase) },
        { "ShakeScreen",              API_FN_PAIR(ShakeScreen) },
        { "ShakeScreenBackground",    API_FN_PAIR(ShakeScreenBackground) },
        { "SkipCutscene",             API_FN_PAIR(SkipCutscene) },
        { "SkipUntilCharacterStops",  API_FN_PAIR(SkipUntilCharacterStops) },
        { "StartCutscene",            API_FN_PAIR(StartCutscene) },
        { "StopChannel",              API_FN_PAIR(stop_and_destroy_channel) },
        { "TintScreen",               API_FN_PAIR(TintScreen) },
        { "UnPauseGame",              API_FN_PAIR(UnPauseGame) },
        { "UpdateInventory",          API_FN_PAIR(UpdateInventory) },
        { "UpdatePalette",            API_FN_PAIR(UpdatePalette) },
        { "Wait",                     API_FN_PAIR(scrWait) },
        { "WaitKey",                  API_FN_PAIR(WaitKey) },
        { "WaitMouse",                API_FN_PAIR(WaitMouse) },
        { "WaitMouseKey",             API_FN_PAIR(WaitMouseKey) },
        { "WaitInput",                API_FN_PAIR(WaitInput) },
        { "SkipWait",                 API_FN_PAIR(SkipWait) },
    };

    ccAddExternalFunctions(global_api);

    // Few functions have to be selected based on API level,
    // We have to do this because AGS compiler did not generate
    // "name^argnum" symbol id for non-member functions for some reason....
    if (base_api < kScriptAPI_v362)
    {
        ScFnRegister global_api_dlgs[] = {
            { "SaveGameSlot",           API_FN_PAIR(SaveGameSlot2) },
            { "RestoreGameDialog",      API_FN_PAIR(restore_game_dialog) },
            { "SaveGameDialog",         API_FN_PAIR(save_game_dialog) },
        };
        ccAddExternalFunctions(global_api_dlgs);
    }
    else
    {
        ScFnRegister global_api_dlgs[] = {
            { "SaveGameSlot",           API_FN_PAIR(SaveGameSlot) },
            { "RestoreGameDialog",      API_FN_PAIR(restore_game_dialog2) },
            { "SaveGameDialog",         API_FN_PAIR(save_game_dialog2) },
        };
        ccAddExternalFunctions(global_api_dlgs);
    }
}
