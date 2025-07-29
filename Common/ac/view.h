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
#ifndef __AC_VIEW_H
#define __AC_VIEW_H

#include <vector>
#include "gfx/gfx_def.h"

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

struct ViewFrame
{
    int   pic = 0;
    int16_t xoffs = 0;
    int16_t yoffs = 0;
    int16_t speed = 0;
    Common::SpriteTransformFlags flags = Common::kSprTf_None;
    int   sound = -1;  // play sound when this frame comes round
    int   reserved_for_future[2] = { 0 }; // kept only for plugin api // CLNUP: may remove in ags4?
    ViewFrame() = default;

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out) const;
};

#define LOOPFLAG_RUNNEXTLOOP 1

struct ViewLoopNew
{
    int   numFrames;
    int   flags;
    std::vector<ViewFrame> frames;
    // NOTE: we still need numFrames:
    // as we always allocate at least 1 frame for safety, to avoid crashes,
    // but have to report "logical" number of frames for the engine API.

    ViewLoopNew();
    void Initialize(int frameCount);
    void Dispose();
    // Tells if this loop has a "Run next loop after this" flag
    bool RunNextLoop() const;
    void WriteToFile(Common::Stream *out) const;
    void ReadFromFile(Common::Stream *in);
    void WriteFrames(Common::Stream *out) const;
    void ReadFrames(Common::Stream *in);
};

struct ViewStruct
{
    int numLoops;
    std::vector<ViewLoopNew> loops;

    ViewStruct();
    void Initialize(int loopCount);
    void Dispose();
    void WriteToFile(Common::Stream *out) const;
    void ReadFromFile(Common::Stream *in);
};

#endif // __AC_VIEW_H
