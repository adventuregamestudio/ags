
#include <stdio.h>
#include <stdlib.h>
#include "ac/view.h"
#include "util/wgt2allg.h"
#include "util/file.h"

void ViewFrame::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    pic = getw(fp);
    xoffs = getshort(fp);//__getshort__bigendian(fp);
    yoffs = getshort(fp);//__getshort__bigendian(fp);
    speed = getshort(fp);//__getshort__bigendian(fp);
    fseek(fp, 2, SEEK_CUR);
    flags = getw(fp);
    sound = getw(fp);
    reserved_for_future[0] = getw(fp);
    reserved_for_future[1] = getw(fp);
//#else
//    pic = getw(fp);
//    fread(&xoffs, 2, 1, fp);
//    fread(&yoffs, 2, 1, fp);
//    fread(&speed, 2, 1, fp);
//    fseek(fp, 2, SEEK_CUR);
//    flags = getw(fp);
//    sound = getw(fp);
//    reserved_for_future[0] = getw(fp);
//    reserved_for_future[1] = getw(fp);
//#endif
}

void ViewFrame::WriteToFile(FILE *fp)
{
    char padding[3] = {0,0,0};

    putw(pic, fp);
    putshort(xoffs, fp);//__getshort__bigendian(fp);
    putshort(yoffs, fp);//__getshort__bigendian(fp);
    putshort(speed, fp);//__getshort__bigendian(fp);
    fwrite(padding, sizeof(char), 2, fp);
    putw(flags, fp);
    putw(sound, fp);
    putw(reserved_for_future[0], fp);
    putw(reserved_for_future[1], fp);
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

void ViewLoopNew::WriteToFile(FILE *ooo)
{
    fwrite(&numFrames, sizeof(short), 1, ooo);
    fwrite(&flags, sizeof(int), 1, ooo);
    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].WriteToFile(ooo);
    }
    //fwrite(frames, sizeof(ViewFrame), numFrames, ooo);
}


void ViewLoopNew::ReadFromFile(FILE *iii)
{
//#ifdef ALLEGRO_BIG_ENDIAN

    // [IKM] 2012-06-13
    // A shoutout from earlier days (or years?) of AGS :)
    // (I guess "Steve" is Steve McCrea)

    /* STEVE PLEASE VALIDATE THAT THIS CODE IS OK */

    Initialize(getshort(iii)/*__getshort__bigendian(iii)*/);
    flags = getw(iii);

    for (int i = 0; i < numFrames; ++i)
    {
        frames[i].ReadFromFile(iii);
    }

//#else

//    Initialize(getshort(iii));
//    flags = getw(iii);

//    fread(frames, sizeof(ViewFrame), numFrames, iii);

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

void ViewStruct::WriteToFile(FILE *ooo)
{
    putshort(numLoops, ooo);
    for (int i = 0; i < numLoops; i++)
    {
        loops[i].WriteToFile(ooo);
    }
}

void ViewStruct::ReadFromFile(FILE *iii)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    Initialize(getshort(iii)/*__getshort__bigendian(iii)*/);
//#else
//    Initialize(getshort(iii));
//#endif

    for (int i = 0; i < numLoops; i++)
    {
        loops[i].ReadFromFile(iii);
    }
}

void ViewStruct272::ReadFromFile(FILE *fp)
{
//#ifdef ALLEGRO_BIG_ENDIAN
    numloops = getshort(fp);//__getshort__bigendian(fp);
    for (int i = 0; i < 16; ++i)
    {
        numframes[i] = getshort(fp);//__getshort__bigendian(fp);
    }
    // skip padding if there is any
    fseek(fp, 2*(2 - ((16+1)%2)), SEEK_CUR);
    fread(loopflags, sizeof(int), 16, fp);
    for (int j = 0; j < 16; ++j)
    {
        for (int i = 0; i < 20; ++i)
        {
            frames[j][i].ReadFromFile(fp);
        }
    }
//#else
//    fread(&numloops, 2, 1, fp);
//    for (int i = 0; i < 16; ++i)
//    {
//        fread(&numframes[i], 2, 1, fp);
//    }
//    fseek(fp, 2*(2 - ((16+1)%2)), SEEK_CUR);
//    fread(loopflags, sizeof(int), 16, fp);
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
