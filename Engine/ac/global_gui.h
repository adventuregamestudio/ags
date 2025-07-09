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
#ifndef __AGS_EE_AC__GLOBALGUI_H
#define __AGS_EE_AC__GLOBALGUI_H

// This is an internal script function, and is undocumented.
// It is used by the editor's automatic macro generation.
int  FindGUIID (const char* GUIName);
// Sets GUI visible property on
void InterfaceOn(int ifn);
// Sets GUI visible property off
void InterfaceOff(int ifn);
int  GetTextWidth(const char *text, int fontnum);
int  GetTextHeight(const char *text, int fontnum, int width);
int  GetFontHeight(int fontnum);
int  GetFontLineSpacing(int fontnum);
void DisableInterface();
void EnableInterface();
void DisableInterfaceEx(bool update_cursor);
void EnableInterfaceEx(bool update_cursor);
// Returns 1 if user interface is enabled, 0 if disabled
int  IsInterfaceEnabled();
void SetTextWindowGUI (int guinum);

#endif // __AGS_EE_AC__GLOBALGUI_H
