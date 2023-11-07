//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <string.h>
#include "ac/view.h"
#include "util/stream.h"

using namespace AGS::Common;

ViewFrame::ViewFrame()
    : pic(0)
    , xoffs(0)
    , yoffs(0)
    , speed(0)
    , flags(0)
    , sound(-1)
{
    reserved_for_future[0] = 0;
    reserved_for_future[1] = 0;
}

void ViewFrame::ReadFromFile(Stream *in)
{
    pic = in->ReadInt32();
    xoffs = in->ReadInt16();
    yoffs = in->ReadInt16();
    speed = in->ReadInt16();
    in->ReadInt16(); // alignment padding to int32
    flags = in->ReadInt32();
    sound = in->ReadInt32();
    in->ReadInt32(); // reserved 1
    in->ReadInt32(); // reserved 1
}

void ViewFrame::WriteToFile(Stream *out)
{
    out->WriteInt32(pic);
    out->WriteInt16(xoffs);
    out->WriteInt16(yoffs);
    out->WriteInt16(speed);
    out->WriteInt16(0); // alignment padding to int32
    out->WriteInt32(flags);
    out->WriteInt32(sound);
    out->WriteInt32(0); // reserved 1
    out->WriteInt32(0); // reserved 1
}

ViewLoopNew::ViewLoopNew()
    : numFrames(0)
    , flags(0)
{
}

bool ViewLoopNew::RunNextLoop() 
{
    return (flags & LOOPFLAG_RUNNEXTLOOP);
}

void ViewLoopNew::Initialize(int frameCount)
{
    numFrames = frameCount;
    flags = 0;
    // an extra frame is allocated to prevent crashes with empty loops
    frames.resize(numFrames > 0 ? numFrames : 1);
}

void ViewLoopNew::Dispose()
{
    frames.clear();
    numFrames = 0;
}

void ViewLoopNew::WriteToFile(Stream *out)
{
    out->WriteInt16(static_cast<uint16_t>(numFrames));
    out->WriteInt32(flags);
    WriteFrames(out);
}

void ViewLoopNew::WriteFrames(Stream *out)
{
    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].WriteToFile(out);
    }
}

void ViewLoopNew::ReadFromFile(Stream *in)
{
    Initialize(static_cast<uint16_t>(in->ReadInt16()));
    flags = in->ReadInt32();
    ReadFrames(in);
}

void ViewLoopNew::ReadFrames(Stream *in)
{
    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].ReadFromFile(in);
    }
}

ViewStruct::ViewStruct()
    : numLoops(0)
{
}

void ViewStruct::Initialize(int loopCount)
{
    numLoops = loopCount;
    loops.resize(numLoops);
}

void ViewStruct::Dispose()
{
    loops.clear();
    numLoops = 0;
}

void ViewStruct::WriteToFile(Stream *out)
{
    out->WriteInt16(static_cast<uint16_t>(numLoops));
    for (int i = 0; i < numLoops; i++)
    {
        loops[i].WriteToFile(out);
    }
}

void ViewStruct::ReadFromFile(Stream *in)
{
    Initialize(static_cast<uint16_t>(in->ReadInt16()));

    for (int i = 0; i < numLoops; i++)
    {
        loops[i].ReadFromFile(in);
    }
}
