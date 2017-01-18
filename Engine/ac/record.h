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

#define REC_MOUSECLICK 1
#define REC_MOUSEMOVE  2
#define REC_MOUSEDOWN  3
#define REC_KBHIT      4
#define REC_GETCH      5
#define REC_KEYDOWN    6
#define REC_MOUSEWHEEL 7
#define REC_SPEECHFINISHED 8
#define REC_ENDOFFILE  0x6f

// If this is defined for record unit it will cause endless recursion!
#ifndef IS_RECORD_UNIT
#undef kbhit
#define mgetbutton rec_mgetbutton
#define domouse rec_domouse
#define misbuttondown rec_misbuttondown
#define kbhit rec_kbhit
#define getch rec_getch
#endif

void write_record_event (int evnt, int dlen, short *dbuf);
void disable_replay_playback ();
void done_playback_event (int size);
int  rec_getch ();
int  rec_kbhit ();
int  rec_iskeypressed (int keycode);
int  rec_isSpeechFinished ();
int  rec_misbuttondown (int but);
int  rec_mgetbutton();
void rec_domouse (int what);
int  check_mouse_wheel ();
void start_recording();
void start_replay_record ();
void stop_recording();
void start_playback();
int  my_readkey();
// Clears buffered keypresses and mouse clicks, if any
void clear_input_buffer();

#endif // __AGS_EE_AC__RECORD_H
