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

// If this is defined for record unit it will cause endless recursion!
#ifndef IS_RECORD_UNIT
#undef kbhit
#define mgetbutton rec_mgetbutton
#define domouse rec_domouse
#define misbuttondown rec_misbuttondown
#define kbhit rec_kbhit
#define getch rec_getch
#endif

int  rec_getch ();
int  rec_kbhit ();
int  rec_iskeypressed (int keycode);
int  rec_isSpeechFinished ();
int  rec_misbuttondown (int but);
int  rec_mgetbutton();
void rec_domouse (int what);
int  check_mouse_wheel ();
int  my_readkey();
// Clears buffered keypresses and mouse clicks, if any
void clear_input_buffer();
// Suspends the game until user keypress
// TODO: this function should not normally exist; need to rewrite update loop to support different states
void wait_until_keypress();

#endif // __AGS_EE_AC__RECORD_H
