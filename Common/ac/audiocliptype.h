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

#ifndef __AC_AUDIOCLIPTYPE_H
#define __AC_AUDIOCLIPTYPE_H

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct AudioClipType
{
    int id = -1;
    int reservedChannels = 0;
    int volume_reduction_while_speech_playing = 0;
    int crossfadeSpeed = 0;
    int reservedForFuture = 0;

    AudioClipType() = default;
    AudioClipType(int id_, int chans, int for_speech_reduction)
        : id(id_), reservedChannels(chans), volume_reduction_while_speech_playing(for_speech_reduction)
    {}

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
    void ReadFromSavegame(Common::Stream *in);
    void WriteToSavegame(Common::Stream *out) const;
};

#endif // __AC_AUDIOCLIPTYPE_H