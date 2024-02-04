//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#ifndef __AC_QUEUEDAUDIOITEM_H
#define __AC_QUEUEDAUDIOITEM_H

class SOUNDCLIP;

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct QueuedAudioItem {
    short audioClipIndex;
    short priority;
    bool  repeat;
    SOUNDCLIP *cachedClip;

    void ReadFromSavegame(Common::Stream *in);
    void WriteToSavegame(Common::Stream *out) const;
};

#endif // __AC_QUEUEDAUDIOITEM_H