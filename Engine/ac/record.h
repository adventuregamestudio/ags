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
#ifndef __AGS_EE_AC__RECORD_H
#define __AGS_EE_AC__RECORD_H

#pragma GCC poison kbhit rec_kbhit  rec_mgetbutton rec_domouse  rec_misbuttondown getch rec_getch
#pragma GCC poison check_mouse_wheel clear_input_buffer wait_until_keypress  rec_isSpeechFinished rec_iskeypressed
// #pragma GCC poison domouse mgetbutton misbuttondown

int  ags_getch ();
int  ags_kbhit ();
int  ags_iskeypressed (int keycode);

int  ags_misbuttondown (int but);
int  ags_mgetbutton();
void ags_domouse (int what);
int  ags_check_mouse_wheel ();

// Clears buffered keypresses and mouse clicks, if any
void ags_clear_input_buffer();

#endif // __AGS_EE_AC__RECORD_H
