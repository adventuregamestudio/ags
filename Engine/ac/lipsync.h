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

#ifndef __AC_LIPSYNC_H
#define __AC_LIPSYNC_H

struct SpeechLipSyncLine {
    char  filename[14];
    int  *endtimeoffs;
    short*frame;
    short numPhonemes;
};

#endif // __AC_LIPSYNC_H