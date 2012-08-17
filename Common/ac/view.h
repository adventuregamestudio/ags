#ifndef __AC_VIEW_H
#define __AC_VIEW_H

#define VFLG_FLIPSPRITE 1

struct ViewFrame {
    int   pic;
    short xoffs, yoffs;
    short speed;
    int   flags;
    int   sound;  // play sound when this frame comes round
    int   reserved_for_future[2];
    ViewFrame() { pic = 0; xoffs = 0; yoffs = 0; speed = 0; }

    void ReadFromFile(FILE *fp);
    void WriteToFile(FILE *fp);
};

#define LOOPFLAG_RUNNEXTLOOP 1

struct ViewLoopNew
{
    short numFrames;
    int   flags;
    ViewFrame *frames;

    bool RunNextLoop();

    void Initialize(int frameCount);
    void Dispose();
    void WriteToFile(FILE *ooo);
    void ReadFromFile(FILE *iii);
};

struct ViewStruct
{
    short numLoops;
    ViewLoopNew *loops;

    void Initialize(int loopCount);
    void Dispose();
    void WriteToFile(FILE *ooo);
    void ReadFromFile(FILE *iii);
};

struct ViewStruct272 {
    short     numloops;
    short     numframes[16];
    int       loopflags[16];
    ViewFrame frames[16][20];
    ViewStruct272() { numloops = 0; numframes[0] = 0; }

    void ReadFromFile(FILE *fp);
};

void Convert272ViewsToNew (int numof, ViewStruct272 *oldv, ViewStruct *newv);

#endif // __AC_VIEW_H