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

#ifndef __AC_EVENTBLOCK_H
#define __AC_EVENTBLOCK_H

// [IKM] 2012-06-22:
// The EventBlock struct seem to had been used somewhen in the past,
// but no longer is.
//
#ifdef UNUSED_CODE
#define MAXCOMMANDS 8
struct EventBlock {
    int   list[MAXCOMMANDS];
    int   respond[MAXCOMMANDS];
    int   respondval[MAXCOMMANDS];
    int   data[MAXCOMMANDS];
    int   numcmd;
    short score[MAXCOMMANDS];
};

extern void add_to_eventblock(EventBlock *evpt, int evnt, int whatac, int val1, int data, short scorr);
#endif // UNUSED_CODE

#endif // __AC_EVENTBLOCK_H