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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__GLOBALBUTTON_H
#define __AGS_EE_AC__GLOBALBUTTON_H

void SetButtonText(int guin,int objn, const char*newtx);
void AnimateButton(int guin, int objn, int view, int loop, int speed, int repeat);
int  GetButtonPic(int guin, int objn, int ptype);
void SetButtonPic(int guin,int objn,int ptype,int slotn);

#endif // __AGS_EE_AC__GLOBALBUTTON_H
