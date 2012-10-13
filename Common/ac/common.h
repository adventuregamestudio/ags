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

#ifndef __AC_COMMON_H
#define __AC_COMMON_H

// quit() and update_polled_stuff_if_runtime() are the project-dependent functions,
// they are defined both in Engine.App and AGS.Native.
void quit(char *);
void update_polled_stuff_if_runtime();

extern char *croom_h_copyright;
extern char *game_file_sig;

#define GAME_FILE_VERSION 42

// [ROFLMAO] What the fuck is this? [/ROFLMAO]
// Used alot in numerous modules
//extern int ff;

#endif // __AC_COMMON_H