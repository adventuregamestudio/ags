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

#include <stdio.h>
#include <stdlib.h>
#include "ac/view.h"
#include "util/alignedstream.h"

using AGS::Common::AlignedStream;
using AGS::Common::Stream;

void ViewFrame::ReadFromFile(Stream *in)
{
    pic = in->ReadInt32();
    xoffs = in->ReadInt16();
    yoffs = in->ReadInt16();
    speed = in->ReadInt16();
    flags = in->ReadInt32();
    sound = in->ReadInt32();
    reserved_for_future[0] = in->ReadInt32();
    reserved_for_future[1] = in->ReadInt32();
}

void ViewFrame::WriteToFile(Stream *out)
{
    out->WriteInt32(pic);
    out->WriteInt16(xoffs);
    out->WriteInt16(yoffs);
    out->WriteInt16(speed);
    out->WriteInt32(flags);
    out->WriteInt32(sound);
    out->WriteInt32(reserved_for_future[0]);
    out->WriteInt32(reserved_for_future[1]);
}

bool ViewLoopNew::RunNextLoop() 
{
    return (flags & LOOPFLAG_RUNNEXTLOOP);
}

void ViewLoopNew::Initialize(int frameCount)
{
    numFrames = frameCount;
    flags = 0;
    frames = (ViewFrame*)calloc(numFrames + 1, sizeof(ViewFrame));
}

void ViewLoopNew::Dispose()
{
    if (frames != NULL)
    {
        free(frames);
        frames = NULL;
        numFrames = 0;
    }
}

void ViewLoopNew::WriteToFile_v321(Stream *out)
{
    out->WriteInt16(numFrames);
    out->WriteInt32(flags);
    WriteFrames_Aligned(out);
}

void ViewLoopNew::WriteFrames_Aligned(Stream *out)
{
    AlignedStream align_s(out, Common::kAligned_Write);
    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].WriteToFile(&align_s);
        align_s.Reset();
    }
}

void ViewLoopNew::ReadFromFile_v321(Stream *in)
{
    Initialize(in->ReadInt16());
    flags = in->ReadInt32();
    ReadFrames_Aligned(in);

    // an extra frame is allocated in memory to prevent
    // crashes with empty loops -- set its picture to teh BLUE CUP!!
    frames[numFrames].pic = 0;
}

void ViewLoopNew::ReadFrames_Aligned(Stream *in)
{
    AlignedStream align_s(in, Common::kAligned_Read);
    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].ReadFromFile(&align_s);
        align_s.Reset();
    }
}

void ViewStruct::Initialize(int loopCount)
{
    numLoops = loopCount;
    if (numLoops > 0)
    {
        loops = (ViewLoopNew*)calloc(numLoops, sizeof(ViewLoopNew));
    }
}

void ViewStruct::Dispose()
{
    if (numLoops > 0)
    {
        free(loops);
        numLoops = 0;
    }
}

void ViewStruct::WriteToFile(Stream *out)
{
    out->WriteInt16(numLoops);
    for (int i = 0; i < numLoops; i++)
    {
        loops[i].WriteToFile_v321(out);
    }
}

void ViewStruct::ReadFromFile(Stream *in)
{
    Initialize(in->ReadInt16());

    for (int i = 0; i < numLoops; i++)
    {
        loops[i].ReadFromFile_v321(in);
    }
}

void ViewStruct272::ReadFromFile(Stream *in)
{
    numloops = in->ReadInt16();
    for (int i = 0; i < 16; ++i)
    {
        numframes[i] = in->ReadInt16();
    }
    in->ReadArrayOfInt32(loopflags, 16);
    for (int j = 0; j < 16; ++j)
    {
        for (int i = 0; i < 20; ++i)
        {
            frames[j][i].ReadFromFile(in);
        }
    }
}

void Convert272ViewsToNew (int numof, ViewStruct272 *oldv, ViewStruct *newv) {

    for (int a = 0; a < numof; a++) {
        newv[a].Initialize(oldv[a].numloops);

        for (int b = 0; b < oldv[a].numloops; b++) 
        {
            newv[a].loops[b].Initialize(oldv[a].numframes[b]);

            if ((oldv[a].numframes[b] > 0) &&
                (oldv[a].frames[b][oldv[a].numframes[b] - 1].pic == -1))
            {
                newv[a].loops[b].flags = LOOPFLAG_RUNNEXTLOOP;
                newv[a].loops[b].numFrames--;
            }
            else
                newv[a].loops[b].flags = 0;

            for (int c = 0; c < newv[a].loops[b].numFrames; c++)
                newv[a].loops[b].frames[c] = oldv[a].frames[b][c];
        }
    }
}
