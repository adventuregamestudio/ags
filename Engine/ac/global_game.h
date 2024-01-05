//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALGAME_H
#define __AGS_EE_AC__GLOBALGAME_H

#include <time.h>
#include "util/string.h"
using namespace AGS; // FIXME later

struct SaveListItem
{
    int    Slot;
    Common::String Description;
    time_t FileTime = 0;

    SaveListItem(int slot, const Common::String &desc, time_t ft)
        : Slot(slot), Description(desc), FileTime(ft) {}

    inline bool operator < (const SaveListItem &other) const
    {
        return FileTime < other.FileTime;
    }
};


// Notify the running game that the engine requested immediate stop
void AbortGame();
void GiveScore(int amnt);
void restart_game();
void RestoreGameSlot(int slnum);
void DeleteSaveSlot (int slnum);
int  GetSaveSlotDescription(int slnum,char*desbuf);
int  LoadSaveSlotScreenshot(int slnum, int width, int height);
void FillSaveList(std::vector<SaveListItem> &saves, unsigned top_index, size_t max_count = -1);
// Find the latest save slot, returns the slot index or -1 at failure
int  GetLastSaveSlot();
void PauseGame();
void UnPauseGame();
int  IsGamePaused();
void SetGlobalInt(int index,int valu);
int  GetGlobalInt(int index);
void SetGlobalString (int index, const char *newval);
void GetGlobalString (int index, char *strval);
int  RunAGSGame(const Common::String &newgame, unsigned int mode, int data);
int  GetGameParameter (int parm, int data1, int data2, int data3);
void QuitGame(int dialog);
void SetRestartPoint();
void SetGameSpeed(int newspd);
int  GetGameSpeed();
int  SetGameOption (int opt, int setting);
int  GetGameOption (int opt);

void SkipUntilCharacterStops(int cc);
void EndSkippingUntilCharStops();
// skipwith decides how it can be skipped:
// 1 = ESC only
// 2 = any key
// 3 = mouse button
// 4 = mouse button or any key
// 5 = right click or ESC only
void StartCutscene (int skipwith);
int EndCutscene ();
// Tell the game to skip current cutscene
void SkipCutscene();

void sc_inputbox(const char*msg,char*bufr);

int GetLocationType(int xxx,int yyy);
void SaveCursorForLocationChange();
void GetLocationName(int xxx,int yyy,char*tempo);

int IsKeyPressed (int keycode);

int SaveScreenShot(const char*namm);
void SetMultitasking (int mode);

void RoomProcessClick(int xx,int yy,int mood);
int IsInteractionAvailable (int xx,int yy,int mood);

void GetMessageText (int msg, char *buffer);

void SetSpeechFont (int fontnum);
void SetNormalFont (int fontnum);

void _sc_AbortGame(const char* text);

int GetGraphicalVariable (const char *varName);
void SetGraphicalVariable (const char *varName, int p_value);
void scrWait(int nloops);
int WaitKey(int nloops);
int WaitMouse(int nloops);
int WaitMouseKey(int nloops);
int WaitInput(int input_flags, int nloops);
void SkipWait();

void scStartRecording(int keyToStop);

#endif // __AGS_EE_AC__GLOBALGAME_H
