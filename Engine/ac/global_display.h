//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALDISPLAY_H
#define __AGS_EE_AC__GLOBALDISPLAY_H

#include "ac/speech.h"

void Display(const char*texx, ...); // applies translation
void DisplaySimple(const char* text); // does not apply translation
void DisplayMB(const char* text); // forces standard Display message box
void DisplayAt(int xxp,int yyp,int widd, const char*text);
void DisplayAtY (int ypos, const char *texx);
void DisplayTopBar(int ypos, int ttexcol, int backcol, const char *title, const char *text);
void DisplaySpeechAt(int xx, int yy, int wii, int aschar, const char *spch);
int  DisplaySpeechBackground(int charid, const char *speel);// [DEPRECATED] but still used by Character_SayBackground

#endif // __AGS_EE_AC__GLOBALDISPLAY_H
