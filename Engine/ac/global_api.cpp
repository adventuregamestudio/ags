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

#include "ac/cdaudio.h"
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
#include "ac/global_drawingsurface.h"
#include "ac/global_dynamicsprite.h"
#include "ac/global_file.h"
#include "ac/global_game.h"
#include "ac/global_gui.h"
#include "ac/global_hotspot.h"
#include "ac/global_inventoryitem.h"
#include "ac/global_invwindow.h"
#include "ac/global_label.h"
#include "ac/global_listbox.h"
#include "ac/global_mouse.h"
#include "ac/global_object.h"
#include "ac/global_overlay.h"
#include "ac/global_palette.h"
#include "ac/global_parser.h"
#include "ac/global_record.h"
#include "ac/global_region.h"
#include "ac/global_room.h"
#include "ac/global_slider.h"
#include "ac/global_screen.h"
#include "ac/global_string.h"
#include "ac/global_textbox.h"
#include "ac/global_timer.h"
#include "ac/global_translation.h"
#include "ac/global_video.h"
#include "ac/global_viewframe.h"
#include "ac/global_viewport.h"
#include "ac/global_walkablearea.h"
#include "ac/global_walkbehind.h"
#include "ac/math.h"
#include "ac/mouse.h"
#include "ac/parser.h"
#include "ac/string.h"
#include "media/audio/audio.h"

using AGS::Common::DataStream;

#include "ac/dynobj/scriptstring.h"
extern ScriptString myScriptStringImpl;

// void _sc_AbortGame(char*texx, ...)
RuntimeScriptValue Sc_sc_AbortGame(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void (int inum)
RuntimeScriptValue Sc_add_inventory(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(add_inventory)
}

// void (int charid, int inum)
RuntimeScriptValue Sc_AddInventoryToCharacter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(AddInventoryToCharacter)
}

// void (int guin, int objn, int view, int loop, int speed, int repeat)
RuntimeScriptValue Sc_AnimateButton(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT6(AnimateButton)
}

// void  (int chh, int loopn, int sppd, int rept)
RuntimeScriptValue Sc_scAnimateCharacter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(scAnimateCharacter)
}

// void (int chh, int loopn, int sppd, int rept, int direction, int blocking)
RuntimeScriptValue Sc_AnimateCharacterEx(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT6(AnimateCharacterEx)
}

// void (int obn,int loopn,int spdd,int rept)
RuntimeScriptValue Sc_AnimateObject(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(AnimateObject)
}

// void (int obn,int loopn,int spdd,int rept, int direction, int blocking)
RuntimeScriptValue Sc_AnimateObjectEx(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT6(AnimateObjectEx)
}

// int (int cchar1,int cchar2)
RuntimeScriptValue Sc_AreCharactersColliding(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(AreCharactersColliding)
}

// int (int charid,int objid)
RuntimeScriptValue Sc_AreCharObjColliding(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(AreCharObjColliding)
}

// int (int obj1,int obj2)
RuntimeScriptValue Sc_AreObjectsColliding(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(AreObjectsColliding)
}

// int (int thing1, int thing2)
RuntimeScriptValue Sc_AreThingsOverlapping(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(AreThingsOverlapping)
}

// void  (int value)
RuntimeScriptValue Sc_CallRoomScript(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(CallRoomScript)
}

// int (int cmdd,int datt)
RuntimeScriptValue Sc_cd_manager(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(cd_manager)
}

// void  (int ifn)
RuntimeScriptValue Sc_CentreGUI(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(CentreGUI)
}

// void (int chaa,int vii)
RuntimeScriptValue Sc_ChangeCharacterView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(ChangeCharacterView)
}

// void  (int curs, int newslot)
RuntimeScriptValue Sc_ChangeCursorGraphic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(ChangeCursorGraphic)
}

// void  (int curs, int x, int y)
RuntimeScriptValue Sc_ChangeCursorHotspot(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ChangeCursorHotspot)
}

// void ()
RuntimeScriptValue Sc_ClaimEvent(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(ClaimEvent)
}

// int (int xx,int yy,int slott,int trans)
RuntimeScriptValue Sc_CreateGraphicOverlay(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT4(CreateGraphicOverlay)
}

// int CreateTextOverlay(int xx,int yy,int wii,int fontid,int clr,char*texx, ...)
RuntimeScriptValue Sc_CreateTextOverlay(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void (int strt,int eend)
RuntimeScriptValue Sc_CyclePalette(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(CyclePalette)
}

// void (int cmdd,int dataa)
RuntimeScriptValue Sc_script_debug(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(script_debug)
}

// void  (int slnum)
RuntimeScriptValue Sc_DeleteSaveSlot(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DeleteSaveSlot)
}

// void  (int gotSlot)
RuntimeScriptValue Sc_free_dynamic_sprite(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(free_dynamic_sprite)
}

// void (int modd)
RuntimeScriptValue Sc_disable_cursor_mode(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(disable_cursor_mode)
}

// void (int alsoEffects)
RuntimeScriptValue Sc_DisableGroundLevelAreas(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DisableGroundLevelAreas)
}

// void (int hsnum)
RuntimeScriptValue Sc_DisableHotspot(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DisableHotspot)
}

// void ()
RuntimeScriptValue Sc_DisableInterface(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(DisableInterface)
}

// void (int hsnum)
RuntimeScriptValue Sc_DisableRegion(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DisableRegion)
}

// void Display(char*texx, ...)
RuntimeScriptValue Sc_Display(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void DisplayAt(int xxp,int yyp,int widd,char*texx, ...)
RuntimeScriptValue Sc_DisplayAt(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void  (int ypos, char *texx)
RuntimeScriptValue Sc_DisplayAtY(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(DisplayAtY, const char)
}

// void (int msnum)
RuntimeScriptValue Sc_DisplayMessage(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(DisplayMessage)
}

// void (int msnum, int ypos)
RuntimeScriptValue Sc_DisplayMessageAtY(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(DisplayMessageAtY)
}

// void (int ypos, int ttexcol, int backcol, char *title, int msgnum)
RuntimeScriptValue Sc_DisplayMessageBar(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3_POBJ_PINT(DisplayMessageBar, const char)
}

// void __sc_displayspeech(int chid,char*texx, ...)
RuntimeScriptValue Sc_sc_displayspeech(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void  (int xx, int yy, int wii, int aschar, char*spch)
RuntimeScriptValue Sc_DisplaySpeechAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4_POBJ(DisplaySpeechAt, const char)
}

// int (int charid,char*speel)
RuntimeScriptValue Sc_DisplaySpeechBackground(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(DisplaySpeechBackground, const char)
}

// void DisplayThought(int chid, const char*texx, ...)
RuntimeScriptValue Sc_DisplayThought(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void DisplayTopBar(int ypos, int ttexcol, int backcol, char *title, char*texx, ...)
RuntimeScriptValue Sc_DisplayTopBar(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void (int modd)
RuntimeScriptValue Sc_enable_cursor_mode(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(enable_cursor_mode)
}

// void ()
RuntimeScriptValue Sc_EnableGroundLevelAreas(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(EnableGroundLevelAreas)
}

// void (int hsnum)
RuntimeScriptValue Sc_EnableHotspot(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(EnableHotspot)
}

// void ()
RuntimeScriptValue Sc_EnableInterface(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(EnableInterface)
}

// void (int hsnum)
RuntimeScriptValue Sc_EnableRegion(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(EnableRegion)
}

// int  ()
RuntimeScriptValue Sc_EndCutscene(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(EndCutscene)
}

// void (int cha,int toface)
RuntimeScriptValue Sc_FaceCharacter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(FaceCharacter)
}

// void (int cha, int xx, int yy)
RuntimeScriptValue Sc_FaceLocation(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(FaceLocation)
}

// void (int sppd)
RuntimeScriptValue Sc_FadeIn(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(FadeIn)
}

// void (int spdd)
RuntimeScriptValue Sc_my_fade_out(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(my_fade_out)
}

// void (DataStream *hha)
RuntimeScriptValue Sc_FileClose(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(FileClose, DataStream)
}

// int  (DataStream *haa)
RuntimeScriptValue Sc_FileIsEOF(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(FileIsEOF, DataStream)
}

// int (DataStream *haa)
RuntimeScriptValue Sc_FileIsError(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(FileIsError, DataStream)
}

// DataStream *(const char*fnmm, const char* cmode)
RuntimeScriptValue Sc_FileOpenCMode(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_STOBJ_POBJ2(DataStream, FileOpenCMode, const char, const char)
}

// void (DataStream *haa,char*toread)
RuntimeScriptValue Sc_FileRead(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(FileRead, DataStream, char)
}

// int (DataStream *haa)
RuntimeScriptValue Sc_FileReadInt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(FileReadInt, DataStream)
}

// char (DataStream *haa)
RuntimeScriptValue Sc_FileReadRawChar(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(FileReadRawChar, DataStream)
}

// int (DataStream *haa)
RuntimeScriptValue Sc_FileReadRawInt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(FileReadRawInt, DataStream)
}

// void (DataStream *haa, const char *towrite)
RuntimeScriptValue Sc_FileWrite(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(FileWrite, DataStream, const char)
}

// void (DataStream *haa,int into)
RuntimeScriptValue Sc_FileWriteInt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT(FileWriteInt, DataStream)
}

// void (DataStream *haa, int chartoWrite)
RuntimeScriptValue Sc_FileWriteRawChar(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT(FileWriteRawChar, DataStream)
}

// void (DataStream *haa, const char*towrite)
RuntimeScriptValue Sc_FileWriteRawLine(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(FileWriteRawLine, DataStream, const char)
}

// int  (const char* GUIName)
RuntimeScriptValue Sc_FindGUIID(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(FindGUIID, const char)
}

// void (int amount)
RuntimeScriptValue Sc_FlipScreen(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(FlipScreen)
}

// int (SCRIPT_FLOAT(value), int roundDirection)
RuntimeScriptValue Sc_FloatToInt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(FloatToInt);
}

// void (int who, int tofollow)
RuntimeScriptValue Sc_FollowCharacter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(FollowCharacter)
}

// void (int who, int tofollow, int distaway, int eagerness)
RuntimeScriptValue Sc_FollowCharacterEx(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(FollowCharacterEx)
}

// int ()
RuntimeScriptValue Sc_GetBackgroundFrame(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetBackgroundFrame);
}

// int (int guin, int objn, int ptype)
RuntimeScriptValue Sc_GetButtonPic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(GetButtonPic);
}

// int  (int xx, int yy)
RuntimeScriptValue Sc_GetCharacterAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetCharacterAt);
}

// int  (int cha, const char *property)
RuntimeScriptValue Sc_GetCharacterProperty(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT_POBJ(GetCharacterProperty, const char);
}

// void  (int item, const char *property, char *bufer)
RuntimeScriptValue Sc_GetCharacterPropertyText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ2(GetCharacterPropertyText, const char, char)
}

// int ()
RuntimeScriptValue Sc_GetCurrentMusic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetCurrentMusic);
}

// int ()
RuntimeScriptValue Sc_GetCursorMode(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetCursorMode);
}

// int  (int dlg, int opt)
RuntimeScriptValue Sc_GetDialogOption(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetDialogOption);
}

// int  (int opt)
RuntimeScriptValue Sc_GetGameOption(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetGameOption);
}

// int  (int parm, int data1, int data2, int data3)
RuntimeScriptValue Sc_GetGameParameter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT4(GetGameParameter);
}

// int ()
RuntimeScriptValue Sc_GetGameSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetGameSpeed);
}

// int (int index)
RuntimeScriptValue Sc_GetGlobalInt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetGlobalInt);
}

// void  (int index, char *strval)
RuntimeScriptValue Sc_GetGlobalString(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(GetGlobalString, char)
}

// int  (const char *varName)
RuntimeScriptValue Sc_GetGraphicalVariable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(GetGraphicalVariable, const char)
}

// int  (int xx,int yy)
RuntimeScriptValue Sc_GetGUIAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetGUIAt);
}

// int  (int xx, int yy)
RuntimeScriptValue Sc_GetGUIObjectAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetGUIObjectAt);
}

// int (int xxx,int yyy)
RuntimeScriptValue Sc_GetHotspotAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetHotspotAt);
}

// void (int hotspot, char *buffer)
RuntimeScriptValue Sc_GetHotspotName(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(GetHotspotName, char)
}

// int  (int hotspot)
RuntimeScriptValue Sc_GetHotspotPointX(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetHotspotPointX);
}

// int  (int hotspot)
RuntimeScriptValue Sc_GetHotspotPointY(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetHotspotPointY);
}

// int  (int hss, const char *property)
RuntimeScriptValue Sc_GetHotspotProperty(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT_POBJ(GetHotspotProperty, const char)
}

// void  (int item, const char *property, char *bufer)
RuntimeScriptValue Sc_GetHotspotPropertyText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ2(GetHotspotPropertyText, const char, char)
}

// int  (int xxx, int yyy)
RuntimeScriptValue Sc_GetInvAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetInvAt);
}

// int (int indx)
RuntimeScriptValue Sc_GetInvGraphic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetInvGraphic);
}

// void (int indx,char*buff)
RuntimeScriptValue Sc_GetInvName(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(GetInvName, char)
}

// int  (int item, const char *property)
RuntimeScriptValue Sc_GetInvProperty(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT_POBJ(GetInvProperty, const char)
}

// void  (int item, const char *property, char *bufer)
RuntimeScriptValue Sc_GetInvPropertyText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ2(GetInvPropertyText, const char, char)
}

// void (int xxx,int yyy,char*tempo)
RuntimeScriptValue Sc_GetLocationName(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(GetLocationName, char)
}

// int (int xxx,int yyy)
RuntimeScriptValue Sc_GetLocationType(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetLocationType);
}

// void  (int msg, char *buffer)
RuntimeScriptValue Sc_GetMessageText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(GetMessageText, char)
}

// int  ()
RuntimeScriptValue Sc_GetMIDIPosition(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetMIDIPosition);
}

// int  ()
RuntimeScriptValue Sc_GetMP3PosMillis(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetMP3PosMillis);
}

// int (int xx,int yy)
RuntimeScriptValue Sc_GetObjectAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetObjectAt);
}

// int (int obn)
RuntimeScriptValue Sc_GetObjectBaseline(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetObjectBaseline);
}

// int (int obn)
RuntimeScriptValue Sc_GetObjectGraphic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetObjectGraphic);
}

// void (int obj, char *buffer)
RuntimeScriptValue Sc_GetObjectName(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(GetObjectName, char)
}

// int  (int hss, const char *property)
RuntimeScriptValue Sc_GetObjectProperty(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT_POBJ(GetObjectProperty, const char)
}

// void  (int item, const char *property, char *bufer)
RuntimeScriptValue Sc_GetObjectPropertyText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ2(GetObjectPropertyText, const char, char)
}

// int  (int objj)
RuntimeScriptValue Sc_GetObjectX(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetObjectX);
}

// int  (int objj)
RuntimeScriptValue Sc_GetObjectY(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(GetObjectY);
}

// int ()
RuntimeScriptValue Sc_GetPlayerCharacter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetPlayerCharacter);
}

// int  ()
RuntimeScriptValue Sc_GetRawTime(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetRawTime);
}

// int  (int xxx, int yyy)
RuntimeScriptValue Sc_GetRegionAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetRegionAt);
}

// int  (const char *property)
RuntimeScriptValue Sc_GetRoomProperty(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(GetRoomProperty, const char)
}

// void  (const char *property, char *bufer)
RuntimeScriptValue Sc_GetRoomPropertyText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(GetRoomPropertyText, const char, char)
}

// int (int slnum,char*desbuf)
RuntimeScriptValue Sc_GetSaveSlotDescription(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT_POBJ(GetSaveSlotDescription, char)
}

// int  (int x, int y)
RuntimeScriptValue Sc_GetScalingAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetScalingAt);
}

// int (int guin,int objn)
RuntimeScriptValue Sc_GetSliderValue(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetSliderValue);
}

// void (int guin, int objn, char*txbuf)
RuntimeScriptValue Sc_GetTextBoxText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(GetTextBoxText, char)
}

// int (char *text, int fontnum, int width)
RuntimeScriptValue Sc_GetTextHeight(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ_PINT2(GetTextHeight, const char)
}

// int (char *text, int fontnum)
RuntimeScriptValue Sc_GetTextWidth(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ_PINT(GetTextWidth, const char)
}

// int (int whatti)
RuntimeScriptValue Sc_sc_GetTime(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(sc_GetTime);
}

// char * (const char *text)
RuntimeScriptValue Sc_get_translation(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_POBJ(char, myScriptStringImpl, get_translation, const char)
}

// int  (char* buffer)
RuntimeScriptValue Sc_GetTranslationName(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(GetTranslationName, char)
}

// int  ()
RuntimeScriptValue Sc_GetViewportX(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetViewportX);
}

// int  ()
RuntimeScriptValue Sc_GetViewportY(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(GetViewportY);
}

// int (int xxx,int yyy)
RuntimeScriptValue Sc_GetWalkableAreaAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(GetWalkableAreaAt);
}

// void (int amnt) 
RuntimeScriptValue Sc_GiveScore(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(GiveScore)
}

// int (int roomnum)
RuntimeScriptValue Sc_HasPlayerBeenInRoom(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(HasPlayerBeenInRoom);
}

// void  () 
RuntimeScriptValue Sc_HideMouseCursor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(HideMouseCursor)
}

// void (const char*msg,char*bufr)
RuntimeScriptValue Sc_sc_inputbox(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(sc_inputbox, const char, char)
}

// void (int ifn)
RuntimeScriptValue Sc_InterfaceOff(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(InterfaceOff)
}

// void (int ifn)
RuntimeScriptValue Sc_InterfaceOn(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(InterfaceOn)
}

// FLOAT_RETURN_TYPE (int value) 
RuntimeScriptValue Sc_IntToFloat(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IntToFloat);
}

// void ()
RuntimeScriptValue Sc_sc_invscreen(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(sc_invscreen)
}

// int (int which)
RuntimeScriptValue Sc_IsButtonDown(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsButtonDown);
}

// int (int chan)
RuntimeScriptValue Sc_IsChannelPlaying(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsChannelPlaying);
}

// int ()
RuntimeScriptValue Sc_IsGamePaused(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsGamePaused);
}

// int  (int guinum)
RuntimeScriptValue Sc_IsGUIOn(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsGUIOn);
}

// int  (int xx,int yy,int mood)
RuntimeScriptValue Sc_IsInteractionAvailable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(IsInteractionAvailable);
}

// int  (int item, int mood)
RuntimeScriptValue Sc_IsInventoryInteractionAvailable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(IsInventoryInteractionAvailable);
}

// int ()
RuntimeScriptValue Sc_IsInterfaceEnabled(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsInterfaceEnabled);
}

// int  (int keycode)
RuntimeScriptValue Sc_IsKeyPressed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsKeyPressed);
}

// int ()
RuntimeScriptValue Sc_IsMusicPlaying(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsMusicPlaying);
}

// int  ()
RuntimeScriptValue Sc_IsMusicVoxAvailable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsMusicVoxAvailable);
}

// int (int objj)
RuntimeScriptValue Sc_IsObjectAnimating(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsObjectAnimating);
}

// int (int objj)
RuntimeScriptValue Sc_IsObjectMoving(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsObjectMoving);
}

// int  (int objj)
RuntimeScriptValue Sc_IsObjectOn(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsObjectOn);
}

// int (int ovrid)
RuntimeScriptValue Sc_IsOverlayValid(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsOverlayValid);
}

// int ()
RuntimeScriptValue Sc_IsSoundPlaying(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsSoundPlaying);
}

// int (int tnum)
RuntimeScriptValue Sc_IsTimerExpired(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(IsTimerExpired);
}

// int  ()
RuntimeScriptValue Sc_IsTranslationAvailable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsTranslationAvailable);
}

// int ()
RuntimeScriptValue Sc_IsVoxAvailable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT(IsVoxAvailable);
}

// void (int guin, int objn, const char*newitem)
RuntimeScriptValue Sc_ListBoxAdd(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(ListBoxAdd, const char)
}

// void (int guin, int objn)
RuntimeScriptValue Sc_ListBoxClear(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(ListBoxClear)
}

// void  (int guin, int objn, const char*filemask)
RuntimeScriptValue Sc_ListBoxDirList(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(ListBoxDirList, const char)
}

// char* (int guin, int objn, int item, char*buffer)
RuntimeScriptValue Sc_ListBoxGetItemText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJ_PINT3_POBJ(char, myScriptStringImpl, ListBoxGetItemText, char)
}

// int (int guin, int objn)
RuntimeScriptValue Sc_ListBoxGetNumItems(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(ListBoxGetNumItems);
}

// int (int guin, int objn)
RuntimeScriptValue Sc_ListBoxGetSelected(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(ListBoxGetSelected)
}

// void (int guin, int objn, int itemIndex)
RuntimeScriptValue Sc_ListBoxRemove(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ListBoxRemove)
}

// int  (int guin, int objn)
RuntimeScriptValue Sc_ListBoxSaveGameList(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(ListBoxSaveGameList);
}

// void (int guin, int objn, int newsel)
RuntimeScriptValue Sc_ListBoxSetSelected(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ListBoxSetSelected)
}

// void  (int guin, int objn, int item)
RuntimeScriptValue Sc_ListBoxSetTopItem(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ListBoxSetTopItem)
}

// int (const char *filename)
RuntimeScriptValue Sc_LoadImageFile(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(LoadImageFile, const char)
}

// int (int slnum, int width, int height)
RuntimeScriptValue Sc_LoadSaveSlotScreenshot(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT3(LoadSaveSlotScreenshot);
}

// void (int inum)
RuntimeScriptValue Sc_lose_inventory(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(lose_inventory)
}

// void (int charid, int inum)
RuntimeScriptValue Sc_LoseInventoryFromCharacter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(LoseInventoryFromCharacter)
}

// void (int obn)
RuntimeScriptValue Sc_MergeObject(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(MergeObject)
}

// void (int cc,int xx,int yy)
RuntimeScriptValue Sc_MoveCharacter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(MoveCharacter)
}

// void (int chaa,int xx,int yy,int direct)
RuntimeScriptValue Sc_MoveCharacterBlocking(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(MoveCharacterBlocking)
}

// void (int cc,int xx, int yy)
RuntimeScriptValue Sc_MoveCharacterDirect(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(MoveCharacterDirect)
}

// void  (int chac, int tox, int toy)
RuntimeScriptValue Sc_MoveCharacterPath(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(MoveCharacterPath)
}

// void (int cc,int xx, int yy)
RuntimeScriptValue Sc_MoveCharacterStraight(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(MoveCharacterStraight)
}

// void (int chaa,int hotsp)
RuntimeScriptValue Sc_MoveCharacterToHotspot(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(MoveCharacterToHotspot)
}

// void (int chaa,int obbj)
RuntimeScriptValue Sc_MoveCharacterToObject(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(MoveCharacterToObject)
}

// void (int objj,int xx,int yy,int spp)
RuntimeScriptValue Sc_MoveObject(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(MoveObject)
}

// void (int objj,int xx,int yy,int spp)
RuntimeScriptValue Sc_MoveObjectDirect(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(MoveObjectDirect)
}

// void (int ovrid, int newx,int newy)
RuntimeScriptValue Sc_MoveOverlay(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(MoveOverlay)
}

// void (int charid)
RuntimeScriptValue Sc_MoveToWalkableArea(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(MoveToWalkableArea)
}

// void (int nrnum)
RuntimeScriptValue Sc_NewRoom(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(NewRoom)
}

// void (int nrnum,int newx,int newy)
RuntimeScriptValue Sc_NewRoomEx(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(NewRoomEx)
}

// void (int charid, int nrnum, int newx, int newy)
RuntimeScriptValue Sc_NewRoomNPC(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(NewRoomNPC)
}

// void (int obn)
RuntimeScriptValue Sc_ObjectOff(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ObjectOff)
}

// void (int obn)
RuntimeScriptValue Sc_ObjectOn(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ObjectOn)
}

// void  (char*text)
RuntimeScriptValue Sc_ParseText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(ParseText, const char)
}

// void ()
RuntimeScriptValue Sc_PauseGame(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(PauseGame)
}

// void  (int channel, int sndnum, int vol, int x, int y)
RuntimeScriptValue Sc_PlayAmbientSound(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(PlayAmbientSound)
}

// void (int numb,int playflags)
RuntimeScriptValue Sc_play_flc_file(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(play_flc_file)
}

// void  (char *filename)
RuntimeScriptValue Sc_PlayMP3File(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(PlayMP3File, const char)
}

// void (int newmus)
RuntimeScriptValue Sc_PlayMusicResetQueue(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(PlayMusicResetQueue)
}

// int (int musnum)
RuntimeScriptValue Sc_PlayMusicQueued(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(PlayMusicQueued)
}

// void  (int mnum)
RuntimeScriptValue Sc_PlaySilentMIDI(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(PlaySilentMIDI)
}

// int (int val1)
RuntimeScriptValue Sc_play_sound(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(play_sound)
}

// int (int val1, int channel)
RuntimeScriptValue Sc_PlaySoundEx(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT2(PlaySoundEx)
}

// void (int who, int which)
RuntimeScriptValue Sc_scr_play_speech(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(__scr_play_speech)
}

// void (const char* name, int skip, int flags)
RuntimeScriptValue Sc_scrPlayVideo(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT2(scrPlayVideo, const char)
}

// void (int xx,int yy,int mood)
RuntimeScriptValue Sc_ProcessClick(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ProcessClick)
}

// void (int dialog)
RuntimeScriptValue Sc_QuitGame(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(QuitGame)
}

// int (int upto)
RuntimeScriptValue Sc_Rand(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(__Rand)
}

// void  (int clr)
RuntimeScriptValue Sc_RawClear(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RawClear)
}

// void  (int xx, int yy, int rad)
RuntimeScriptValue Sc_RawDrawCircle(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(RawDrawCircle)
}

// void  (int frame, int translev)
RuntimeScriptValue Sc_RawDrawFrameTransparent(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RawDrawFrameTransparent)
}

// void (int xx, int yy, int slot)
RuntimeScriptValue Sc_RawDrawImage(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(RawDrawImageCore)
}

// void (int xx, int yy, int slot)
RuntimeScriptValue Sc_RawDrawImageOffset(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(RawDrawImageOffset)
}

// void (int xx, int yy, int gotSlot, int width, int height)
RuntimeScriptValue Sc_RawDrawImageResized(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(RawDrawImageResized)
}

// void (int xx, int yy, int slot, int trans)
RuntimeScriptValue Sc_RawDrawImageTransparent(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(RawDrawImageTransparent)
}

// void  (int fromx, int fromy, int tox, int toy)
RuntimeScriptValue Sc_RawDrawLine(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(RawDrawLine)
}

// void (int x1, int y1, int x2, int y2)
RuntimeScriptValue Sc_RawDrawRectangle(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(RawDrawRectangle)
}

// void (int x1, int y1, int x2, int y2, int x3, int y3)
RuntimeScriptValue Sc_RawDrawTriangle(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT6(RawDrawTriangle)
}

// void RawPrint (int xx, int yy, char*texx, ...)
RuntimeScriptValue Sc_RawPrint(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void  (int xx, int yy, int wid, int font, int msgm)
RuntimeScriptValue Sc_RawPrintMessageWrapped(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(RawPrintMessageWrapped)
}

// void ()
RuntimeScriptValue Sc_RawRestoreScreen(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(RawRestoreScreen)
}

// void (int red, int green, int blue, int opacity)
RuntimeScriptValue Sc_RawRestoreScreenTinted(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(RawRestoreScreenTinted)
}

// void  ()
RuntimeScriptValue Sc_RawSaveScreen(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(RawSaveScreen)
}

// void  (int clr)
RuntimeScriptValue Sc_RawSetColor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RawSetColor)
}

// void (int red, int grn, int blu)
RuntimeScriptValue Sc_RawSetColorRGB(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(RawSetColorRGB)
}

// void ()
RuntimeScriptValue Sc_RefreshMouse(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(RefreshMouse)
}

// void (int chat)
RuntimeScriptValue Sc_ReleaseCharacterView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ReleaseCharacterView)
}

// void ()
RuntimeScriptValue Sc_ReleaseViewport(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(ReleaseViewport)
}

// void (int obj)
RuntimeScriptValue Sc_RemoveObjectTint(RuntimeScriptValue *params, int32_t param_count)
{

    API_SCALL_VOID_PINT(RemoveObjectTint)
}

// void (int ovrid)
RuntimeScriptValue Sc_RemoveOverlay(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RemoveOverlay)
}

// void (int areanum)
RuntimeScriptValue Sc_RemoveWalkableArea(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RemoveWalkableArea)
}

// void (int nrnum)
RuntimeScriptValue Sc_ResetRoom(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ResetRoom)
}

// void ()
RuntimeScriptValue Sc_restart_game(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(restart_game)
}

// void ()
RuntimeScriptValue Sc_restore_game_dialog(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(restore_game_dialog)
}

// void (int slnum)
RuntimeScriptValue Sc_RestoreGameSlot(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RestoreGameSlot)
}

// void (int areanum)
RuntimeScriptValue Sc_RestoreWalkableArea(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RestoreWalkableArea)
}

// int  (char *newgame, unsigned int mode, int data)
RuntimeScriptValue Sc_RunAGSGame(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ_PINT2(RunAGSGame, const char)
}

// void  (int cc, int mood)
RuntimeScriptValue Sc_RunCharacterInteraction(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunCharacterInteraction)
}

// void (int tum)
RuntimeScriptValue Sc_RunDialog(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(RunDialog)
}

// void  (int hotspothere, int mood)
RuntimeScriptValue Sc_RunHotspotInteraction(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunHotspotInteraction)
}

// void  (int iit, int modd)
RuntimeScriptValue Sc_RunInventoryInteraction(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunInventoryInteraction)
}

// void  (int aa, int mood)
RuntimeScriptValue Sc_RunObjectInteraction(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunObjectInteraction)
}

// void  (int regnum, int mood)
RuntimeScriptValue Sc_RunRegionInteraction(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(RunRegionInteraction)
}

// int  (char*checkwords)
RuntimeScriptValue Sc_Said(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(Said, const char)
}

// int  (char*buffer)
RuntimeScriptValue Sc_SaidUnknownWord(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(SaidUnknownWord, char)
}

// void ()
RuntimeScriptValue Sc_SaveCursorForLocationChange(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SaveCursorForLocationChange)
}

// void ()
RuntimeScriptValue Sc_save_game_dialog(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(save_game_dialog)
}

// void (int slotn, const char*descript)
RuntimeScriptValue Sc_save_game(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(save_game, const char)
}

// int (char*namm)
RuntimeScriptValue Sc_SaveScreenShot(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(SaveScreenShot, const char)
}

// void  (int position)
RuntimeScriptValue Sc_SeekMIDIPosition(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SeekMIDIPosition)
}

// void (int patnum)
RuntimeScriptValue Sc_SeekMODPattern(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SeekMODPattern)
}

// void  (int posn)
RuntimeScriptValue Sc_SeekMP3PosMillis(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SeekMP3PosMillis)
}

// void (int iit)
RuntimeScriptValue Sc_SetActiveInventory(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetActiveInventory)
}

// void  (int red, int green, int blue, int opacity, int luminance)
RuntimeScriptValue Sc_SetAmbientTint(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(SetAmbientTint)
}

// void (int area, int brightness)
RuntimeScriptValue Sc_SetAreaLightLevel(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetAreaLightLevel)
}

// void (int area, int min, int max)
RuntimeScriptValue Sc_SetAreaScaling(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetAreaScaling)
}

// void (int frnum)
RuntimeScriptValue Sc_SetBackgroundFrame(RuntimeScriptValue *params, int32_t param_count)
{
     API_SCALL_VOID_PINT(SetBackgroundFrame)
}

// void (int guin,int objn,int ptype,int slotn)
RuntimeScriptValue Sc_SetButtonPic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetButtonPic)
}

// void (int guin,int objn,char*newtx)
RuntimeScriptValue Sc_SetButtonText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(SetButtonText, const char)
}

// void (int chan, int newvol)
RuntimeScriptValue Sc_SetChannelVolume(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetChannelVolume)
}

// void  (int obn, int basel)
RuntimeScriptValue Sc_SetCharacterBaseline(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetCharacterBaseline)
}

// void  (int cha, int clik)
RuntimeScriptValue Sc_SetCharacterClickable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetCharacterClickable)
}

// void (int chaa, int view, int loop, int frame)
RuntimeScriptValue Sc_SetCharacterFrame(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetCharacterFrame)
}

// void (int who, int iview, int itime)
RuntimeScriptValue Sc_SetCharacterIdle(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetCharacterIdle)
}

// void  (int who, int yesorno)
RuntimeScriptValue Sc_SetCharacterIgnoreLight(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetCharacterIgnoreLight)
}

// void  (int cha, int clik)
RuntimeScriptValue Sc_SetCharacterIgnoreWalkbehinds(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetCharacterIgnoreWalkbehinds)
}

// void  (int who, int flag, int yesorno)
RuntimeScriptValue Sc_SetCharacterProperty(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetCharacterProperty)
}

// void  (int chaa, int vii, int intrv)
RuntimeScriptValue Sc_SetCharacterBlinkView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetCharacterBlinkView)
}

// void  (int chaa, int vii)
RuntimeScriptValue Sc_SetCharacterSpeechView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetCharacterSpeechView)
}

// void (int chaa,int nspeed)
RuntimeScriptValue Sc_SetCharacterSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetCharacterSpeed)
}

// void (int chaa, int xspeed, int yspeed)
RuntimeScriptValue Sc_SetCharacterSpeedEx(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetCharacterSpeedEx)
}

// void (int obn,int trans)
RuntimeScriptValue Sc_SetCharacterTransparency(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetCharacterTransparency)
}

// void (int chaa,int vii)
RuntimeScriptValue Sc_SetCharacterView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetCharacterView)
}

// void  (int chaa, int vii, int loop, int align)
RuntimeScriptValue Sc_SetCharacterViewEx(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetCharacterViewEx)
}

// void  (int chaa, int vii, int xoffs, int yoffs)
RuntimeScriptValue Sc_SetCharacterViewOffset(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetCharacterViewOffset)
}

// void (int newmode)
RuntimeScriptValue Sc_set_cursor_mode(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(set_cursor_mode)
}

// void ()
RuntimeScriptValue Sc_set_default_cursor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(set_default_cursor)
}

// void (int dlg,int opt,int onoroff)
RuntimeScriptValue Sc_SetDialogOption(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetDialogOption)
}

// void  (int newvol)
RuntimeScriptValue Sc_SetDigitalMasterVolume(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetDigitalMasterVolume)
}

// void (int red, int green, int blue)
RuntimeScriptValue Sc_SetFadeColor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetFadeColor)
}

// void  (int vii, int loop, int frame, int sound)
RuntimeScriptValue Sc_SetFrameSound(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetFrameSound)
}

// int  (int opt, int setting)
RuntimeScriptValue Sc_SetGameOption(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGameOption)
}

// void (int newspd)
RuntimeScriptValue Sc_SetGameSpeed(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetGameSpeed)
}

// void (int index,int valu)
RuntimeScriptValue Sc_SetGlobalInt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGlobalInt)
}

// void  (int index, const char *newval)
RuntimeScriptValue Sc_SetGlobalString(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(SetGlobalString, const char)
}

// void  (const char *varName, int p_value)
RuntimeScriptValue Sc_SetGraphicalVariable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT(SetGraphicalVariable, const char)
}

// void  (int guin, int slotn)
RuntimeScriptValue Sc_SetGUIBackgroundPic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGUIBackgroundPic)
}

// void (int guin, int clickable)
RuntimeScriptValue Sc_SetGUIClickable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGUIClickable)
}

// void (int guin, int objn, int enabled)
RuntimeScriptValue Sc_SetGUIObjectEnabled(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetGUIObjectEnabled)
}

// void (int guin, int objn, int xx, int yy)
RuntimeScriptValue Sc_SetGUIObjectPosition(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetGUIObjectPosition)
}

// void (int ifn, int objn, int newwid, int newhit)
RuntimeScriptValue Sc_SetGUIObjectSize(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetGUIObjectSize)
}

// void (int ifn,int xx,int yy)
RuntimeScriptValue Sc_SetGUIPosition(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetGUIPosition)
}

// void  (int ifn, int widd, int hitt)
RuntimeScriptValue Sc_SetGUISize(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetGUISize)
}

// void (int ifn, int trans)
RuntimeScriptValue Sc_SetGUITransparency(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGUITransparency)
}

// void (int guin, int z)
RuntimeScriptValue Sc_SetGUIZOrder(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetGUIZOrder)
}

// void (int invi, const char *newName)
RuntimeScriptValue Sc_SetInvItemName(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT_POBJ(SetInvItemName, const char)
}

// void (int invi, int piccy)
RuntimeScriptValue Sc_set_inv_item_pic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(set_inv_item_pic)
}

// void (int ww,int hh)
RuntimeScriptValue Sc_SetInvDimensions(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetInvDimensions)
}

// void (int guin,int objn, int colr)
RuntimeScriptValue Sc_SetLabelColor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetLabelColor)
}

// void (int guin,int objn, int fontnum)
RuntimeScriptValue Sc_SetLabelFont(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetLabelFont)
}

// void (int guin,int objn,char*newtx)
RuntimeScriptValue Sc_SetLabelText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(SetLabelText, const char)
}

// void  (int x1, int y1, int x2, int y2)
RuntimeScriptValue Sc_SetMouseBounds(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetMouseBounds)
}

// void (int newcurs)
RuntimeScriptValue Sc_set_mouse_cursor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(set_mouse_cursor)
}

// void  (int newx, int newy)
RuntimeScriptValue Sc_SetMousePosition(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetMousePosition)
}

// void  (int mode)
RuntimeScriptValue Sc_SetMultitasking(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetMultitasking)
}

// void (int newvol)
RuntimeScriptValue Sc_SetMusicMasterVolume(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetMusicMasterVolume)
}

// void (int loopflag)
RuntimeScriptValue Sc_SetMusicRepeat(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetMusicRepeat)
}

// void (int newvol)
RuntimeScriptValue Sc_SetMusicVolume(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetMusicVolume)
}

// void  ()
RuntimeScriptValue Sc_SetNextCursor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SetNextCursor)
}

// void (int newtrans)
RuntimeScriptValue Sc_SetNextScreenTransition(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetNextScreenTransition)
}

// void  (int fontnum)
RuntimeScriptValue Sc_SetNormalFont(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetNormalFont)
}

// void  (int obn, int basel)
RuntimeScriptValue Sc_SetObjectBaseline(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectBaseline)
}

// void  (int cha, int clik)
RuntimeScriptValue Sc_SetObjectClickable(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectClickable)
}

// void (int obn,int viw,int lop,int fra)
RuntimeScriptValue Sc_SetObjectFrame(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetObjectFrame)
}

// void (int obn,int slott) 
RuntimeScriptValue Sc_SetObjectGraphic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectGraphic)
}

// void  (int cha, int clik)
RuntimeScriptValue Sc_SetObjectIgnoreWalkbehinds(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectIgnoreWalkbehinds)
}

// void (int objj, int tox, int toy)
RuntimeScriptValue Sc_SetObjectPosition(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetObjectPosition)
}

// void (int obj, int red, int green, int blue, int opacity, int luminance)
RuntimeScriptValue Sc_SetObjectTint(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT6(SetObjectTint)
}

// void (int obn,int trans)
RuntimeScriptValue Sc_SetObjectTransparency(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectTransparency)
}

// void (int obn,int vii)
RuntimeScriptValue Sc_SetObjectView(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetObjectView)
}

// void (int inndx,int rr,int gg,int bb)
RuntimeScriptValue Sc_SetPalRGB(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT4(SetPalRGB)
}

// void (int newchar)
RuntimeScriptValue Sc_SetPlayerCharacter(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetPlayerCharacter)
}

// void  (int area, int red, int green, int blue, int amount)
RuntimeScriptValue Sc_SetRegionTint(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT5(SetRegionTint)
}

// void ()
RuntimeScriptValue Sc_SetRestartPoint(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(SetRestartPoint)
}

// void (int newtrans)
RuntimeScriptValue Sc_SetScreenTransition(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetScreenTransition)
}

// void  (int newval)
RuntimeScriptValue Sc_SetSkipSpeech(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSkipSpeech)
}

// void (int guin,int objn, int valn)
RuntimeScriptValue Sc_SetSliderValue(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetSliderValue)
}

// void (int newvol)
RuntimeScriptValue Sc_SetSoundVolume(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSoundVolume)
}

// void  (int fontnum)
RuntimeScriptValue Sc_SetSpeechFont(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSpeechFont)
}

// void  (int newstyle)
RuntimeScriptValue Sc_SetSpeechStyle(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSpeechStyle)
}

// void (int newvol)
RuntimeScriptValue Sc_SetSpeechVolume(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetSpeechVolume)
}

// void (int chaa,int ncol)
RuntimeScriptValue Sc_SetTalkingColor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetTalkingColor)
}

// void (int guin,int objn, int fontnum)
RuntimeScriptValue Sc_SetTextBoxFont(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(SetTextBoxFont)
}

// void (int guin, int objn, char*txbuf)
RuntimeScriptValue Sc_SetTextBoxText(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2_POBJ(SetTextBoxText, const char)
}

// void SetTextOverlay(int ovrid,int xx,int yy,int wii,int fontid,int clr,char*texx,...)
RuntimeScriptValue Sc_SetTextOverlay(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// void  (int guinum)
RuntimeScriptValue Sc_SetTextWindowGUI(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetTextWindowGUI)
}

// void (int tnum,int timeout)
RuntimeScriptValue Sc_script_SetTimer(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(script_SetTimer)
}

// void (int offsx,int offsy)
RuntimeScriptValue Sc_SetViewport(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetViewport)
}

// void  (int newmod)
RuntimeScriptValue Sc_SetVoiceMode(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SetVoiceMode)
}

// void (int wa,int bl)
RuntimeScriptValue Sc_SetWalkBehindBase(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT2(SetWalkBehindBase)
}

// void (int severe)
RuntimeScriptValue Sc_ShakeScreen(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(ShakeScreen)
}

// void  (int delay, int amount, int length)
RuntimeScriptValue Sc_ShakeScreenBackground(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(ShakeScreenBackground)
}

// void  ()
RuntimeScriptValue Sc_ShowMouseCursor(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(ShowMouseCursor)
}

// void (int cc)
RuntimeScriptValue Sc_SkipUntilCharacterStops(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(SkipUntilCharacterStops)
}

// void  (int skipwith)
RuntimeScriptValue Sc_StartCutscene(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StartCutscene)
}

// void  (int keyToStop)
RuntimeScriptValue Sc_scStartRecording(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(scStartRecording)
}

// void  (int channel)
RuntimeScriptValue Sc_StopAmbientSound(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StopAmbientSound)
}

// void  (int chid) 
RuntimeScriptValue Sc_stop_and_destroy_channel(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(stop_and_destroy_channel)
}

// void ()
RuntimeScriptValue Sc_StopDialog(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(StopDialog)
}

// void (int chaa)
RuntimeScriptValue Sc_StopMoving(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StopMoving)
}

// void ()
RuntimeScriptValue Sc_scr_StopMusic(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(scr_StopMusic)
}

// void (int objj)
RuntimeScriptValue Sc_StopObjectMoving(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(StopObjectMoving)
}

// void (char*s1,char*s2)
RuntimeScriptValue Sc_sc_strcat(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(_sc_strcat, char, const char)
}

RuntimeScriptValue Sc_stricmp(RuntimeScriptValue *params, int32_t param_count)
{
    // Calling C stdlib function stricmp
    API_SCALL_INT_POBJ2(stricmp, const char, const char)
}

RuntimeScriptValue Sc_strcmp(RuntimeScriptValue *params, int32_t param_count)
{
    // Calling C stdlib function strcmp
    API_SCALL_INT_POBJ2(strcmp, const char, const char)
}

// int  (const char *s1, const char *s2)
RuntimeScriptValue Sc_StrContains(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ2(StrContains, const char, const char)
}

// void (char*s1, const char*s2);
RuntimeScriptValue Sc_sc_strcpy(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ2(_sc_strcpy, char, const char)
}

// void (char*destt, const char*texx, ...);
RuntimeScriptValue Sc_sc_sprintf(RuntimeScriptValue *params, int32_t param_count)
{
    // TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    return RuntimeScriptValue();
}

// int  (char *strin, int posn)
RuntimeScriptValue Sc_StrGetCharAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ_PINT(StrGetCharAt, const char)
}

// int (const char*stino)
RuntimeScriptValue Sc_StringToInt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_POBJ(StringToInt, const char)
}

RuntimeScriptValue Sc_strlen(RuntimeScriptValue *params, int32_t param_count)
{
    // Calling C stdlib function strlen
    API_SCALL_INT_POBJ(strlen, const char)
}

// void  (char *strin, int posn, int nchar)
RuntimeScriptValue Sc_StrSetCharAt(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ_PINT2(StrSetCharAt, char)
}

// void  (char *desbuf)
RuntimeScriptValue Sc_sc_strlower(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(_sc_strlower, char)
}

// void  (char *desbuf)
RuntimeScriptValue Sc_sc_strupper(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_POBJ(_sc_strupper, char)
}

// void (int red, int grn, int blu)
RuntimeScriptValue Sc_TintScreen(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT3(TintScreen)
}

// void ()
RuntimeScriptValue Sc_UnPauseGame(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(UnPauseGame)
}

// void ()
RuntimeScriptValue Sc_update_invorder(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(update_invorder)
}

// void ()
RuntimeScriptValue Sc_UpdatePalette(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID(UpdatePalette)
}

// void (int nloops)
RuntimeScriptValue Sc_scrWait(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_VOID_PINT(scrWait)
}

// int (int nloops)
RuntimeScriptValue Sc_WaitKey(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(WaitKey)
}

// int (int nloops)
RuntimeScriptValue Sc_WaitMouseKey(RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_INT_PINT(WaitMouseKey)
}


void RegisterGlobalAPI()
{
    ccAddExternalStaticFunction("AbortGame",                Sc_sc_AbortGame);
	ccAddExternalStaticFunction("AddInventory",             Sc_add_inventory);
	ccAddExternalStaticFunction("AddInventoryToCharacter",  Sc_AddInventoryToCharacter);
	ccAddExternalStaticFunction("AnimateButton",            Sc_AnimateButton);
	ccAddExternalStaticFunction("AnimateCharacter",         Sc_scAnimateCharacter);
	ccAddExternalStaticFunction("AnimateCharacterEx",       Sc_AnimateCharacterEx);
	ccAddExternalStaticFunction("AnimateObject",            Sc_AnimateObject);
	ccAddExternalStaticFunction("AnimateObjectEx",          Sc_AnimateObjectEx);
	ccAddExternalStaticFunction("AreCharactersColliding",   Sc_AreCharactersColliding);
	ccAddExternalStaticFunction("AreCharObjColliding",      Sc_AreCharObjColliding);
	ccAddExternalStaticFunction("AreObjectsColliding",      Sc_AreObjectsColliding);
	ccAddExternalStaticFunction("AreThingsOverlapping",     Sc_AreThingsOverlapping);
	ccAddExternalStaticFunction("CallRoomScript",           Sc_CallRoomScript);
	ccAddExternalStaticFunction("CDAudio",                  Sc_cd_manager);
	ccAddExternalStaticFunction("CentreGUI",                Sc_CentreGUI);
	ccAddExternalStaticFunction("ChangeCharacterView",      Sc_ChangeCharacterView);
	ccAddExternalStaticFunction("ChangeCursorGraphic",      Sc_ChangeCursorGraphic);
	ccAddExternalStaticFunction("ChangeCursorHotspot",      Sc_ChangeCursorHotspot);
	ccAddExternalStaticFunction("ClaimEvent",               Sc_ClaimEvent);
	ccAddExternalStaticFunction("CreateGraphicOverlay",     Sc_CreateGraphicOverlay);
	ccAddExternalStaticFunction("CreateTextOverlay",        Sc_CreateTextOverlay);
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
	ccAddExternalStaticFunction("DisplaySpeech",            Sc_sc_displayspeech);
	ccAddExternalStaticFunction("DisplaySpeechAt",          Sc_DisplaySpeechAt);
	ccAddExternalStaticFunction("DisplaySpeechBackground",  Sc_DisplaySpeechBackground);
	ccAddExternalStaticFunction("DisplayThought",           Sc_DisplayThought);
	ccAddExternalStaticFunction("DisplayTopBar",            Sc_DisplayTopBar);
	ccAddExternalStaticFunction("EnableCursorMode",         Sc_enable_cursor_mode);
	ccAddExternalStaticFunction("EnableGroundLevelAreas",   Sc_EnableGroundLevelAreas);
	ccAddExternalStaticFunction("EnableHotspot",            Sc_EnableHotspot);
	ccAddExternalStaticFunction("EnableInterface",          Sc_EnableInterface);
	ccAddExternalStaticFunction("EnableRegion",             Sc_EnableRegion);
	ccAddExternalStaticFunction("EndCutscene",              Sc_EndCutscene);
	ccAddExternalStaticFunction("FaceCharacter",            Sc_FaceCharacter);
	ccAddExternalStaticFunction("FaceLocation",             Sc_FaceLocation);
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
	ccAddExternalStaticFunction("FollowCharacter",          Sc_FollowCharacter);
	ccAddExternalStaticFunction("FollowCharacterEx",        Sc_FollowCharacterEx);
	ccAddExternalStaticFunction("GetBackgroundFrame",       Sc_GetBackgroundFrame);
	ccAddExternalStaticFunction("GetButtonPic",             Sc_GetButtonPic);
	ccAddExternalStaticFunction("GetCharacterAt",           Sc_GetCharacterAt);
	ccAddExternalStaticFunction("GetCharacterProperty",     Sc_GetCharacterProperty);
	ccAddExternalStaticFunction("GetCharacterPropertyText", Sc_GetCharacterPropertyText);
	ccAddExternalStaticFunction("GetCurrentMusic",          Sc_GetCurrentMusic);
	ccAddExternalStaticFunction("GetCursorMode",            Sc_GetCursorMode);
	ccAddExternalStaticFunction("GetDialogOption",          Sc_GetDialogOption);
	ccAddExternalStaticFunction("GetGameOption",            Sc_GetGameOption);
	ccAddExternalStaticFunction("GetGameParameter",         Sc_GetGameParameter);
	ccAddExternalStaticFunction("GetGameSpeed",             Sc_GetGameSpeed);
	ccAddExternalStaticFunction("GetGlobalInt",             Sc_GetGlobalInt);
	ccAddExternalStaticFunction("GetGlobalString",          Sc_GetGlobalString);
	ccAddExternalStaticFunction("GetGraphicalVariable",     Sc_GetGraphicalVariable);
	ccAddExternalStaticFunction("GetGUIAt",                 Sc_GetGUIAt);
	ccAddExternalStaticFunction("GetGUIObjectAt",           Sc_GetGUIObjectAt);
	ccAddExternalStaticFunction("GetHotspotAt",             Sc_GetHotspotAt);
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
	//ccAddExternalStaticFunction("GetLanguageString",      Sc_GetLanguageString);
	ccAddExternalStaticFunction("GetLocationName",          Sc_GetLocationName);
	ccAddExternalStaticFunction("GetLocationType",          Sc_GetLocationType);
	ccAddExternalStaticFunction("GetMessageText",           Sc_GetMessageText);
	ccAddExternalStaticFunction("GetMIDIPosition",          Sc_GetMIDIPosition);
	ccAddExternalStaticFunction("GetMP3PosMillis",          Sc_GetMP3PosMillis);
	ccAddExternalStaticFunction("GetObjectAt",              Sc_GetObjectAt);
	ccAddExternalStaticFunction("GetObjectBaseline",        Sc_GetObjectBaseline);
	ccAddExternalStaticFunction("GetObjectGraphic",         Sc_GetObjectGraphic);
	ccAddExternalStaticFunction("GetObjectName",            Sc_GetObjectName);
	ccAddExternalStaticFunction("GetObjectProperty",        Sc_GetObjectProperty);
	ccAddExternalStaticFunction("GetObjectPropertyText",    Sc_GetObjectPropertyText);
	ccAddExternalStaticFunction("GetObjectX",               Sc_GetObjectX);
	ccAddExternalStaticFunction("GetObjectY",               Sc_GetObjectY);
	//  ccAddExternalStaticFunction("GetPalette",           Sc_scGetPal);
	ccAddExternalStaticFunction("GetPlayerCharacter",       Sc_GetPlayerCharacter);
	ccAddExternalStaticFunction("GetRawTime",               Sc_GetRawTime);
	ccAddExternalStaticFunction("GetRegionAt",              Sc_GetRegionAt);
	ccAddExternalStaticFunction("GetRoomProperty",          Sc_GetRoomProperty);
	ccAddExternalStaticFunction("GetRoomPropertyText",      Sc_GetRoomPropertyText);
	ccAddExternalStaticFunction("GetSaveSlotDescription",   Sc_GetSaveSlotDescription);
	ccAddExternalStaticFunction("GetScalingAt",             Sc_GetScalingAt);
	ccAddExternalStaticFunction("GetSliderValue",           Sc_GetSliderValue);
	ccAddExternalStaticFunction("GetTextBoxText",           Sc_GetTextBoxText);
	ccAddExternalStaticFunction("GetTextHeight",            Sc_GetTextHeight);
	ccAddExternalStaticFunction("GetTextWidth",             Sc_GetTextWidth);
	ccAddExternalStaticFunction("GetTime",                  Sc_sc_GetTime);
	ccAddExternalStaticFunction("GetTranslation",           Sc_get_translation);
	ccAddExternalStaticFunction("GetTranslationName",       Sc_GetTranslationName);
	ccAddExternalStaticFunction("GetViewportX",             Sc_GetViewportX);
	ccAddExternalStaticFunction("GetViewportY",             Sc_GetViewportY);
	ccAddExternalStaticFunction("GetWalkableAreaAt",        Sc_GetWalkableAreaAt);
	ccAddExternalStaticFunction("GiveScore",                Sc_GiveScore);
	ccAddExternalStaticFunction("HasPlayerBeenInRoom",      Sc_HasPlayerBeenInRoom);
	ccAddExternalStaticFunction("HideMouseCursor",          Sc_HideMouseCursor);
	ccAddExternalStaticFunction("InputBox",                 Sc_sc_inputbox);
	ccAddExternalStaticFunction("InterfaceOff",             Sc_InterfaceOff);
	ccAddExternalStaticFunction("InterfaceOn",              Sc_InterfaceOn);
	ccAddExternalStaticFunction("IntToFloat",               Sc_IntToFloat);
	ccAddExternalStaticFunction("InventoryScreen",          Sc_sc_invscreen);
	ccAddExternalStaticFunction("IsButtonDown",             Sc_IsButtonDown);
	ccAddExternalStaticFunction("IsChannelPlaying",         Sc_IsChannelPlaying);
	ccAddExternalStaticFunction("IsGamePaused",             Sc_IsGamePaused);
	ccAddExternalStaticFunction("IsGUIOn",                  Sc_IsGUIOn);
	ccAddExternalStaticFunction("IsInteractionAvailable",   Sc_IsInteractionAvailable);
	ccAddExternalStaticFunction("IsInventoryInteractionAvailable", Sc_IsInventoryInteractionAvailable);
	ccAddExternalStaticFunction("IsInterfaceEnabled",       Sc_IsInterfaceEnabled);
	ccAddExternalStaticFunction("IsKeyPressed",             Sc_IsKeyPressed);
	ccAddExternalStaticFunction("IsMusicPlaying",           Sc_IsMusicPlaying);
	ccAddExternalStaticFunction("IsMusicVoxAvailable",      Sc_IsMusicVoxAvailable);
	ccAddExternalStaticFunction("IsObjectAnimating",        Sc_IsObjectAnimating);
	ccAddExternalStaticFunction("IsObjectMoving",           Sc_IsObjectMoving);
	ccAddExternalStaticFunction("IsObjectOn",               Sc_IsObjectOn);
	ccAddExternalStaticFunction("IsOverlayValid",           Sc_IsOverlayValid);
	ccAddExternalStaticFunction("IsSoundPlaying",           Sc_IsSoundPlaying);
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
	ccAddExternalStaticFunction("LoseInventory",            Sc_lose_inventory);
	ccAddExternalStaticFunction("LoseInventoryFromCharacter", Sc_LoseInventoryFromCharacter);
	ccAddExternalStaticFunction("MergeObject",              Sc_MergeObject);
	ccAddExternalStaticFunction("MoveCharacter",            Sc_MoveCharacter);
	ccAddExternalStaticFunction("MoveCharacterBlocking",    Sc_MoveCharacterBlocking);
	ccAddExternalStaticFunction("MoveCharacterDirect",      Sc_MoveCharacterDirect);
	ccAddExternalStaticFunction("MoveCharacterPath",        Sc_MoveCharacterPath);
	ccAddExternalStaticFunction("MoveCharacterStraight",    Sc_MoveCharacterStraight);
	ccAddExternalStaticFunction("MoveCharacterToHotspot",   Sc_MoveCharacterToHotspot);
	ccAddExternalStaticFunction("MoveCharacterToObject",    Sc_MoveCharacterToObject);
	ccAddExternalStaticFunction("MoveObject",               Sc_MoveObject);
	ccAddExternalStaticFunction("MoveObjectDirect",         Sc_MoveObjectDirect);
	ccAddExternalStaticFunction("MoveOverlay",              Sc_MoveOverlay);
	ccAddExternalStaticFunction("MoveToWalkableArea",       Sc_MoveToWalkableArea);
	ccAddExternalStaticFunction("NewRoom",                  Sc_NewRoom);
	ccAddExternalStaticFunction("NewRoomEx",                Sc_NewRoomEx);
	ccAddExternalStaticFunction("NewRoomNPC",               Sc_NewRoomNPC);
	ccAddExternalStaticFunction("ObjectOff",                Sc_ObjectOff);
	ccAddExternalStaticFunction("ObjectOn",                 Sc_ObjectOn);
	ccAddExternalStaticFunction("ParseText",                Sc_ParseText);
	ccAddExternalStaticFunction("PauseGame",                Sc_PauseGame);
	ccAddExternalStaticFunction("PlayAmbientSound",         Sc_PlayAmbientSound);
	ccAddExternalStaticFunction("PlayFlic",                 Sc_play_flc_file);
	ccAddExternalStaticFunction("PlayMP3File",              Sc_PlayMP3File);
	ccAddExternalStaticFunction("PlayMusic",                Sc_PlayMusicResetQueue);
	ccAddExternalStaticFunction("PlayMusicQueued",          Sc_PlayMusicQueued);
	ccAddExternalStaticFunction("PlaySilentMIDI",           Sc_PlaySilentMIDI);
	ccAddExternalStaticFunction("PlaySound",                Sc_play_sound);
	ccAddExternalStaticFunction("PlaySoundEx",              Sc_PlaySoundEx);
	ccAddExternalStaticFunction("PlaySpeech",               Sc_scr_play_speech);
	ccAddExternalStaticFunction("PlayVideo",                Sc_scrPlayVideo);
	ccAddExternalStaticFunction("ProcessClick",             Sc_ProcessClick);
	ccAddExternalStaticFunction("QuitGame",                 Sc_QuitGame);
	ccAddExternalStaticFunction("Random",                   Sc_Rand);
	ccAddExternalStaticFunction("RawClearScreen",           Sc_RawClear);
	ccAddExternalStaticFunction("RawDrawCircle",            Sc_RawDrawCircle);
	ccAddExternalStaticFunction("RawDrawFrameTransparent",  Sc_RawDrawFrameTransparent);
	ccAddExternalStaticFunction("RawDrawImage",             Sc_RawDrawImage);
	ccAddExternalStaticFunction("RawDrawImageOffset",       Sc_RawDrawImageOffset);
	ccAddExternalStaticFunction("RawDrawImageResized",      Sc_RawDrawImageResized);
	ccAddExternalStaticFunction("RawDrawImageTransparent",  Sc_RawDrawImageTransparent);
	ccAddExternalStaticFunction("RawDrawLine",              Sc_RawDrawLine);
	ccAddExternalStaticFunction("RawDrawRectangle",         Sc_RawDrawRectangle);
	ccAddExternalStaticFunction("RawDrawTriangle",          Sc_RawDrawTriangle);
	ccAddExternalStaticFunction("RawPrint",                 Sc_RawPrint);
	ccAddExternalStaticFunction("RawPrintMessageWrapped",   Sc_RawPrintMessageWrapped);
	ccAddExternalStaticFunction("RawRestoreScreen",         Sc_RawRestoreScreen);
	ccAddExternalStaticFunction("RawRestoreScreenTinted",   Sc_RawRestoreScreenTinted);
	ccAddExternalStaticFunction("RawSaveScreen",            Sc_RawSaveScreen);
	ccAddExternalStaticFunction("RawSetColor",              Sc_RawSetColor);
	ccAddExternalStaticFunction("RawSetColorRGB",           Sc_RawSetColorRGB);
	ccAddExternalStaticFunction("RefreshMouse",             Sc_RefreshMouse);
	ccAddExternalStaticFunction("ReleaseCharacterView",     Sc_ReleaseCharacterView);
	ccAddExternalStaticFunction("ReleaseViewport",          Sc_ReleaseViewport);
	ccAddExternalStaticFunction("RemoveObjectTint",         Sc_RemoveObjectTint);
	ccAddExternalStaticFunction("RemoveOverlay",            Sc_RemoveOverlay);
	ccAddExternalStaticFunction("RemoveWalkableArea",       Sc_RemoveWalkableArea);
	ccAddExternalStaticFunction("ResetRoom",                Sc_ResetRoom);
	ccAddExternalStaticFunction("RestartGame",              Sc_restart_game);
	ccAddExternalStaticFunction("RestoreGameDialog",        Sc_restore_game_dialog);
	ccAddExternalStaticFunction("RestoreGameSlot",          Sc_RestoreGameSlot);
	ccAddExternalStaticFunction("RestoreWalkableArea",      Sc_RestoreWalkableArea);
	ccAddExternalStaticFunction("RunAGSGame",               Sc_RunAGSGame);
	ccAddExternalStaticFunction("RunCharacterInteraction",  Sc_RunCharacterInteraction);
	ccAddExternalStaticFunction("RunDialog",                Sc_RunDialog);
	ccAddExternalStaticFunction("RunHotspotInteraction",    Sc_RunHotspotInteraction);
	ccAddExternalStaticFunction("RunInventoryInteraction",  Sc_RunInventoryInteraction);
	ccAddExternalStaticFunction("RunObjectInteraction",     Sc_RunObjectInteraction);
	ccAddExternalStaticFunction("RunRegionInteraction",     Sc_RunRegionInteraction);
	ccAddExternalStaticFunction("Said",                     Sc_Said);
	ccAddExternalStaticFunction("SaidUnknownWord",          Sc_SaidUnknownWord);
	ccAddExternalStaticFunction("SaveCursorForLocationChange", Sc_SaveCursorForLocationChange);
	ccAddExternalStaticFunction("SaveGameDialog",           Sc_save_game_dialog);
	ccAddExternalStaticFunction("SaveGameSlot",             Sc_save_game);
	ccAddExternalStaticFunction("SaveScreenShot",           Sc_SaveScreenShot);
	ccAddExternalStaticFunction("SeekMIDIPosition",         Sc_SeekMIDIPosition);
	ccAddExternalStaticFunction("SeekMODPattern",           Sc_SeekMODPattern);
	ccAddExternalStaticFunction("SeekMP3PosMillis",         Sc_SeekMP3PosMillis);
	ccAddExternalStaticFunction("SetActiveInventory",       Sc_SetActiveInventory);
	ccAddExternalStaticFunction("SetAmbientTint",           Sc_SetAmbientTint);
	ccAddExternalStaticFunction("SetAreaLightLevel",        Sc_SetAreaLightLevel);
	ccAddExternalStaticFunction("SetAreaScaling",           Sc_SetAreaScaling);
	ccAddExternalStaticFunction("SetBackgroundFrame",       Sc_SetBackgroundFrame);
	ccAddExternalStaticFunction("SetButtonPic",             Sc_SetButtonPic);
	ccAddExternalStaticFunction("SetButtonText",            Sc_SetButtonText);
	ccAddExternalStaticFunction("SetChannelVolume",         Sc_SetChannelVolume);
	ccAddExternalStaticFunction("SetCharacterBaseline",     Sc_SetCharacterBaseline);
	ccAddExternalStaticFunction("SetCharacterClickable",    Sc_SetCharacterClickable);
	ccAddExternalStaticFunction("SetCharacterFrame",        Sc_SetCharacterFrame);
	ccAddExternalStaticFunction("SetCharacterIdle",         Sc_SetCharacterIdle);
	ccAddExternalStaticFunction("SetCharacterIgnoreLight",  Sc_SetCharacterIgnoreLight);
	ccAddExternalStaticFunction("SetCharacterIgnoreWalkbehinds", Sc_SetCharacterIgnoreWalkbehinds);
	ccAddExternalStaticFunction("SetCharacterProperty",     Sc_SetCharacterProperty);
	ccAddExternalStaticFunction("SetCharacterBlinkView",    Sc_SetCharacterBlinkView);
	ccAddExternalStaticFunction("SetCharacterSpeechView",   Sc_SetCharacterSpeechView);
	ccAddExternalStaticFunction("SetCharacterSpeed",        Sc_SetCharacterSpeed);
	ccAddExternalStaticFunction("SetCharacterSpeedEx",      Sc_SetCharacterSpeedEx);
	ccAddExternalStaticFunction("SetCharacterTransparency", Sc_SetCharacterTransparency);
	ccAddExternalStaticFunction("SetCharacterView",         Sc_SetCharacterView);
	ccAddExternalStaticFunction("SetCharacterViewEx",       Sc_SetCharacterViewEx);
	ccAddExternalStaticFunction("SetCharacterViewOffset",   Sc_SetCharacterViewOffset);
	ccAddExternalStaticFunction("SetCursorMode",            Sc_set_cursor_mode);
	ccAddExternalStaticFunction("SetDefaultCursor",         Sc_set_default_cursor);
	ccAddExternalStaticFunction("SetDialogOption",          Sc_SetDialogOption);
	ccAddExternalStaticFunction("SetDigitalMasterVolume",   Sc_SetDigitalMasterVolume);
	ccAddExternalStaticFunction("SetFadeColor",             Sc_SetFadeColor);
	ccAddExternalStaticFunction("SetFrameSound",            Sc_SetFrameSound);
	ccAddExternalStaticFunction("SetGameOption",            Sc_SetGameOption);
	ccAddExternalStaticFunction("SetGameSpeed",             Sc_SetGameSpeed);
	ccAddExternalStaticFunction("SetGlobalInt",             Sc_SetGlobalInt);
	ccAddExternalStaticFunction("SetGlobalString",          Sc_SetGlobalString);
	ccAddExternalStaticFunction("SetGraphicalVariable",     Sc_SetGraphicalVariable);
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
	ccAddExternalStaticFunction("SetInvDimensions",         Sc_SetInvDimensions);
	ccAddExternalStaticFunction("SetLabelColor",            Sc_SetLabelColor);
	ccAddExternalStaticFunction("SetLabelFont",             Sc_SetLabelFont);
	ccAddExternalStaticFunction("SetLabelText",             Sc_SetLabelText);
	ccAddExternalStaticFunction("SetMouseBounds",           Sc_SetMouseBounds);
	ccAddExternalStaticFunction("SetMouseCursor",           Sc_set_mouse_cursor);
	ccAddExternalStaticFunction("SetMousePosition",         Sc_SetMousePosition);
	ccAddExternalStaticFunction("SetMultitaskingMode",      Sc_SetMultitasking);
	ccAddExternalStaticFunction("SetMusicMasterVolume",     Sc_SetMusicMasterVolume);
	ccAddExternalStaticFunction("SetMusicRepeat",           Sc_SetMusicRepeat);
	ccAddExternalStaticFunction("SetMusicVolume",           Sc_SetMusicVolume);
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
	//  ccAddExternalStaticFunction("SetPalette",           scSetPal);
	ccAddExternalStaticFunction("SetPalRGB",                Sc_SetPalRGB);
	ccAddExternalStaticFunction("SetPlayerCharacter",       Sc_SetPlayerCharacter);
	ccAddExternalStaticFunction("SetRegionTint",            Sc_SetRegionTint);
	ccAddExternalStaticFunction("SetRestartPoint",          Sc_SetRestartPoint);
	ccAddExternalStaticFunction("SetScreenTransition",      Sc_SetScreenTransition);
	ccAddExternalStaticFunction("SetSkipSpeech",            Sc_SetSkipSpeech);
	ccAddExternalStaticFunction("SetSliderValue",           Sc_SetSliderValue);
	ccAddExternalStaticFunction("SetSoundVolume",           Sc_SetSoundVolume);
	ccAddExternalStaticFunction("SetSpeechFont",            Sc_SetSpeechFont);
	ccAddExternalStaticFunction("SetSpeechStyle",           Sc_SetSpeechStyle);
	ccAddExternalStaticFunction("SetSpeechVolume",          Sc_SetSpeechVolume);
	ccAddExternalStaticFunction("SetTalkingColor",          Sc_SetTalkingColor);
	ccAddExternalStaticFunction("SetTextBoxFont",           Sc_SetTextBoxFont);
	ccAddExternalStaticFunction("SetTextBoxText",           Sc_SetTextBoxText);
	ccAddExternalStaticFunction("SetTextOverlay",           Sc_SetTextOverlay);
	ccAddExternalStaticFunction("SetTextWindowGUI",         Sc_SetTextWindowGUI);
	ccAddExternalStaticFunction("SetTimer",                 Sc_script_SetTimer);
	ccAddExternalStaticFunction("SetViewport",              Sc_SetViewport);
	ccAddExternalStaticFunction("SetVoiceMode",             Sc_SetVoiceMode);
	ccAddExternalStaticFunction("SetWalkBehindBase",        Sc_SetWalkBehindBase);
	ccAddExternalStaticFunction("ShakeScreen",              Sc_ShakeScreen);
	ccAddExternalStaticFunction("ShakeScreenBackground",    Sc_ShakeScreenBackground);
	ccAddExternalStaticFunction("ShowMouseCursor",          Sc_ShowMouseCursor);
	ccAddExternalStaticFunction("SkipUntilCharacterStops",  Sc_SkipUntilCharacterStops);
	ccAddExternalStaticFunction("StartCutscene",            Sc_StartCutscene);
	ccAddExternalStaticFunction("StartRecording",           Sc_scStartRecording);
	ccAddExternalStaticFunction("StopAmbientSound",         Sc_StopAmbientSound);
	ccAddExternalStaticFunction("StopChannel",              Sc_stop_and_destroy_channel);
	ccAddExternalStaticFunction("StopDialog",               Sc_StopDialog);
	ccAddExternalStaticFunction("StopMoving",               Sc_StopMoving);
	ccAddExternalStaticFunction("StopMusic",                Sc_scr_StopMusic);
	ccAddExternalStaticFunction("StopObjectMoving",         Sc_StopObjectMoving);
	ccAddExternalStaticFunction("StrCat",                   Sc_sc_strcat);
	ccAddExternalStaticFunction("StrCaseComp",              Sc_stricmp);
	ccAddExternalStaticFunction("StrComp",                  Sc_strcmp);
	ccAddExternalStaticFunction("StrContains",              Sc_StrContains);
	ccAddExternalStaticFunction("StrCopy",                  Sc_sc_strcpy);
	ccAddExternalStaticFunction("StrFormat",                Sc_sc_sprintf);
	ccAddExternalStaticFunction("StrGetCharAt",             Sc_StrGetCharAt);
	ccAddExternalStaticFunction("StringToInt",              Sc_StringToInt);
	ccAddExternalStaticFunction("StrLen",                   Sc_strlen);
	ccAddExternalStaticFunction("StrSetCharAt",             Sc_StrSetCharAt);
	ccAddExternalStaticFunction("StrToLowerCase",           Sc_sc_strlower);
	ccAddExternalStaticFunction("StrToUpperCase",           Sc_sc_strupper);
	ccAddExternalStaticFunction("TintScreen",               Sc_TintScreen);
	ccAddExternalStaticFunction("UnPauseGame",              Sc_UnPauseGame);
	ccAddExternalStaticFunction("UpdateInventory",          Sc_update_invorder);
	ccAddExternalStaticFunction("UpdatePalette",            Sc_UpdatePalette);
	ccAddExternalStaticFunction("Wait",                     Sc_scrWait);
	ccAddExternalStaticFunction("WaitKey",                  Sc_WaitKey);
	ccAddExternalStaticFunction("WaitMouseKey",             Sc_WaitMouseKey);
}
