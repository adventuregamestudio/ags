
#include <stdio.h>
#include <stdlib.h>
#include "ac/view.h"
#include "util/wgt2allg.h"
#include "util/file.h"
#include "util/datastream.h"

using AGS::Common::CDataStream;

void ViewFrame::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    pic = in->ReadInt32();
    xoffs = in->ReadInt16();//__getshort__bigendian(fp);
    yoffs = in->ReadInt16();//__getshort__bigendian(fp);
    speed = in->ReadInt16();//__getshort__bigendian(fp);
    in->Seek(Common::kSeekCurrent, 2);
    flags = in->ReadInt32();
    sound = in->ReadInt32();
    reserved_for_future[0] = in->ReadInt32();
    reserved_for_future[1] = in->ReadInt32();
//#else
//    pic = in->ReadInt32();
//    in->ReadArray(&xoffs, 2, 1);
//    in->ReadArray(&yoffs, 2, 1);
//    in->ReadArray(&speed, 2, 1);
//    Seek(fp, 2, Common::kSeekCurrent);
//    flags = in->ReadInt32();
//    sound = in->ReadInt32();
//    reserved_for_future[0] = in->ReadInt32();
//    reserved_for_future[1] = in->ReadInt32();
//#endif
}

void ViewFrame::WriteToFile(CDataStream *out)
{
    char padding[3] = {0,0,0};

    out->WriteInt32(pic);
    out->WriteInt16(xoffs);//__getshort__bigendian(fp);
    out->WriteInt16(yoffs);//__getshort__bigendian(fp);
    out->WriteInt16(speed);//__getshort__bigendian(fp);
    out->Write(padding, 2);
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

void ViewLoopNew::WriteToFile(CDataStream *out)
{
    out->WriteInt16(numFrames);
    out->WriteInt32(flags);
    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].WriteToFile(out);
    }
    //out->WriteArray(frames, sizeof(ViewFrame), numFrames);
}


void ViewLoopNew::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN

    // [IKM] 2012-06-13
    // A shoutout from earlier days (or years?) of AGS :)
    // (I guess "Steve" is Steve McCrea)

    /* STEVE PLEASE VALIDATE THAT THIS CODE IS OK */

    Initialize(in->ReadInt16()/*__getshort__bigendian(iii)*/);
    flags = in->ReadInt32();

    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].ReadFromFile(in);
    }

//#else

//    Initialize(->ReadInt16(iii));
//    flags = ->ReadInt32(iii);

//    in->ReadArray(frames, sizeof(ViewFrame), numFrames, iii);

//#endif

    // an extra frame is allocated in memory to prevent
    // crashes with empty loops -- set its picture to teh BLUE CUP!!
    frames[numFrames].pic = 0;
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

void ViewStruct::WriteToFile(CDataStream *out)
{
    out->WriteInt16(numLoops);
    for (int i = 0; i < numLoops; i++)
    {
        loops[i].WriteToFile(out);
    }
}

void ViewStruct::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    Initialize(in->ReadInt16()/*__getshort__bigendian(iii)*/);
//#else
//    Initialize(->ReadInt16(iii));
//#endif

    for (int i = 0; i < numLoops; i++)
    {
        loops[i].ReadFromFile(in);
    }
}

void ViewStruct272::ReadFromFile(CDataStream *in)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    numloops = in->ReadInt16();//__getshort__bigendian(fp);
    for (int i = 0; i < 16; ++i)
    {
        numframes[i] = in->ReadInt16();//__getshort__bigendian(fp);
    }
    // skip padding if there is any
    in->Seek(Common::kSeekCurrent, 2*(2 - ((16+1)%2)));
    in->ReadArrayOfInt32(loopflags, 16);
    for (int j = 0; j < 16; ++j)
    {
        for (int i = 0; i < 20; ++i)
        {
            frames[j][i].ReadFromFile(in);
        }
    }
//#else
//    in->ReadArray(&numloops, 2, 1);
//    for (int i = 0; i < 16; ++i)
//    {
//        in->ReadArray(&numframes[i], 2, 1);
//    }
//    Seek(fp, 2*(2 - ((16+1)%2)), Common::kSeekCurrent);
//    in->ReadArray(loopflags, sizeof(int), 16);
//    for (int j = 0; j < 16; ++j)
//    {
//        for (int i = 0; i < 20; ++i)
//        {
//            frames[j][i].ReadFromFile(fp);
//        }
//    }
//#endif
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
