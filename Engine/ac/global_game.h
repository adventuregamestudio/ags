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
#ifndef __AGS_EE_AC__GLOBALGAME_H
#define __AGS_EE_AC__GLOBALGAME_H

#include <time.h>
#include "ac/runtime_defines.h"
#include "util/string.h"
using namespace AGS; // FIXME later

struct SaveListItem
{
    int    Slot;
    Common::String Description;
    time_t FileTime = 0;

    SaveListItem(int slot, const Common::String &desc, time_t ft)
        : Slot(slot), Description(desc), FileTime(ft) {}
};

//
// SaveListItem comparers, for sorting
//
struct SaveItemCmpByNumber
{
    bool operator()(const SaveListItem &item1, const SaveListItem &item2) const
    {
        return item1.Slot < item2.Slot;
    }
};

struct SaveItemCmpByTime
{
    bool operator()(const SaveListItem &item1, const SaveListItem &item2) const
    {
        return item1.FileTime < item2.FileTime;
    }
};

struct SaveItemCmpByDesc
{
    bool operator()(const SaveListItem &item1, const SaveListItem &item2) const
    {
        return item1.Description.Compare(item2.Description) < 0;
    }
};


// Notify the running game that the engine requested immediate stop
void AbortGame();
void restart_game();
void CopySaveSlot(int old_slot, int new_slot);
void MoveSaveSlot(int old_slot, int new_slot);
void RestoreGameSlot(int slnum);
void MoveSaveSlot (int old_slot, int new_slot);
void SaveGameSlot(int slnum, const char *descript, int spritenum);
void SaveGameSlot2(int slnum, const char *descript);
void DeleteSaveSlot (int slnum);
// Fills a list of SaveListItems by any save files found within the given range
void FillSaveList(std::vector<SaveListItem> &saves, unsigned bot_index, unsigned top_index, bool get_description);
// Fills a list of SaveListItems by any save files found within the given range; sorts the resulting list
void FillSaveList(std::vector<SaveListItem> &saves, unsigned bot_index, unsigned top_index, bool get_description, ScriptSaveGameSortStyle save_sort, ScriptSortDirection sort_dir);
// Fills a list of SaveListItems by any save files found in the source list of slots;
// slots may be listed in any order, and post-sorting is only optional
void FillSaveList(const std::vector<int> &slots, std::vector<SaveListItem> &saves, bool get_description, ScriptSaveGameSortStyle save_sort = kScSaveGameSort_None, ScriptSortDirection sort_dir = kScSortNone);
// Find the latest save slot, returns the slot index or -1 at failure
int  GetLastSaveSlot();
void PauseGame();
void UnPauseGame();
int  IsGamePaused();
int  RunAGSGame(const Common::String &newgame, unsigned int mode, int data);
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

// ShowInputBox assumes a string buffer of MAX_MAXSTRLEN
void ShowInputBox(const char *msg, char *bufr);
void ShowInputBoxImpl(const char *msg, char *bufr, size_t buf_len);

int GetLocationType(int xxx,int yyy);
// Returns the name (description) of a location under given coordinates;
// if nothing found, returns an empty string.
const char *GetLocationName(int xxx, int yyy);
// GetLocationNameInBuf assumes a string buffer of MAX_MAXSTRLEN
void GetLocationNameInBuf(int xxx,int yyy, char *buf);

int IsKeyPressed (int keycode);

int SaveScreenShot(const char*namm);
void SetMultitasking (int mode);

void RoomProcessClick(int xx,int yy,int mood);
int IsInteractionAvailable (int xx,int yy,int mood);

void _sc_AbortGame(const char* text);

void scrWait(int nloops);
int WaitKey(int nloops);
int WaitMouse(int nloops);
int WaitMouseKey(int nloops);
int WaitInput(int input_flags, int nloops);
void SkipWait();

#endif // __AGS_EE_AC__GLOBALGAME_H
