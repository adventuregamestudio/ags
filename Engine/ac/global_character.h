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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALCHARACTER_H
#define __AGS_EE_AC__GLOBALCHARACTER_H

#include "ac/characterinfo.h"

void StopMoving(int chaa);
void ReleaseCharacterView(int chat);
int  GetCharacterWidth(int ww);
int  GetCharacterHeight(int charid);
void MoveCharacterToHotspot(int chaa,int hotsp);

void RunCharacterInteraction (int cc, int mood);// [DEPRECATED]
int  GetPlayerCharacter();

int GetCharacterSpeechAnimationDelay(CharacterInfo *cha);
int GetCharIDAtScreen(int xx, int yy);

void SetActiveInventory(int iit);// [DEPRECATED] but still used in Character_SetAsPlayer
void update_invorder();
void add_inventory(int inum);
void lose_inventory(int inum);

void DisplaySpeechAt (int xx, int yy, int wii, int aschar, const char*spch);
int DisplaySpeechBackground(int charid, const char*speel);// [DEPRECATED] but still used by Character_SayBackground

#endif // __AGS_EE_AC__CHARACTEREXTRAS_H
