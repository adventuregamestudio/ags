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
#ifndef __AC_VIEW_H
#define __AC_VIEW_H

#include <vector>

namespace AGS { namespace Common { class Stream; } }
using namespace AGS; // FIXME later

#define VFLG_FLIPSPRITE 1

struct ViewFrame {
    int   pic;
    short xoffs, yoffs;
    short speed;
    int   flags;  // VFLG_* flags
    int   sound;  // play sound when this frame comes round
    int   reserved_for_future[2]; // kept only for plugin api
    // not saved, set at runtime only
    int   audioclip; // actual audio clip reference (in case sound is a legacy number)
    ViewFrame();

    void ReadFromFile(Common::Stream *in);
    void WriteToFile(Common::Stream *out);
};

#define LOOPFLAG_RUNNEXTLOOP 1

struct ViewLoopNew
{
    int   numFrames;
    int   flags;
    std::vector<ViewFrame> frames;
    // NOTE: we still need numFrames for backward compatibility:
    // some older versions could allocate extra frame(s) for safety,
    // but have to report "logical" number of frames for the engine API.

    ViewLoopNew();
    void Initialize(int frameCount);
    void Dispose();
    bool RunNextLoop();
    void WriteToFile_v321(Common::Stream *out);
    void ReadFromFile_v321(Common::Stream *in);
    void WriteFrames(Common::Stream *out);
    void ReadFrames(Common::Stream *in);
};

struct ViewStruct
{
    int numLoops;
    std::vector<ViewLoopNew> loops;

    ViewStruct();
    void Initialize(int loopCount);
    void Dispose();
    void WriteToFile(Common::Stream *out);
    void ReadFromFile(Common::Stream *in);
};

struct ViewStruct272 {
    short     numloops;
    short     numframes[16];
    int       loopflags[16];
    ViewFrame frames[16][20];

    ViewStruct272();
    void ReadFromFile(Common::Stream *in);
};

void Convert272ViewsToNew(const std::vector<ViewStruct272> &oldv, std::vector<ViewStruct> &newv);

#endif // __AC_VIEW_H