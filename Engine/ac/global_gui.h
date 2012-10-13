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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALGUI_H
#define __AGS_EE_AC__GLOBALGUI_H

int  IsGUIOn (int guinum);
// This is an internal script function, and is undocumented.
// It is used by the editor's automatic macro generation.
int  FindGUIID (const char* GUIName);
void InterfaceOn(int ifn);
void InterfaceOff(int ifn);
void CentreGUI (int ifn);
int  GetTextWidth(char *text, int fontnum);
int  GetTextHeight(char *text, int fontnum, int width);
void SetGUIBackgroundPic (int guin, int slotn);
void DisableInterface();
void EnableInterface();
// Returns 1 if user interface is enabled, 0 if disabled
int  IsInterfaceEnabled();
// pass trans=0 for fully solid, trans=100 for fully transparent
void SetGUITransparency(int ifn, int trans);
void SetGUIClickable(int guin, int clickable);
void SetGUIZOrder(int guin, int z);
void SetGUISize (int ifn, int widd, int hitt);
void SetGUIPosition(int ifn,int xx,int yy);
void SetGUIObjectSize(int ifn, int objn, int newwid, int newhit);
void SetGUIObjectEnabled(int guin, int objn, int enabled);
void SetGUIObjectPosition(int guin, int objn, int xx, int yy);
int GetGUIObjectAt (int xx, int yy);
int GetGUIAt (int xx,int yy);
void SetTextWindowGUI (int guinum);

#endif // __AGS_EE_AC__GLOBALGUI_H
