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

#ifndef __AC_MESSAGEINFO_H
#define __AC_MESSAGEINFO_H

#include "ac/common_defines.h" // PCKD

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#pragma pack(1)

#define MSG_DISPLAYNEXT 1 // supercedes using alt-200 at end of message
#define MSG_TIMELIMIT   2
struct MessageInfo {
    char  displayas  PCKD; // 0 = normal window, 1 = as speech
    char  flags      PCKD; // combination of MSG_xxx flags

    void ReadFromFile(Common::Stream *in);
};

#pragma pack()

#endif // __AC_MESSAGEINFO_H