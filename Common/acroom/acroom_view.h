#ifndef __CROOM_VIEW_H
#define __CROOM_VIEW_H

#define VFLG_FLIPSPRITE 1

struct ViewFrame {
    int   pic;
    short xoffs, yoffs;
    short speed;
    int   flags;
    int   sound;  // play sound when this frame comes round
    int   reserved_for_future[2];
    ViewFrame() { pic = 0; xoffs = 0; yoffs = 0; speed = 0; }

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp)
    {
        pic = getw(fp);
        xoffs = __getshort__bigendian(fp);
        yoffs = __getshort__bigendian(fp);
        speed = __getshort__bigendian(fp);
        fseek(fp, 2, SEEK_CUR);
        flags = getw(fp);
        sound = getw(fp);
        reserved_for_future[0] = getw(fp);
        reserved_for_future[1] = getw(fp);
    }
#else
    void ReadFromFile(FILE *fp)
    {
        pic = getw(fp);
        fread(&xoffs, 2, 1, fp);
        fread(&yoffs, 2, 1, fp);
        fread(&speed, 2, 1, fp);
        fseek(fp, 2, SEEK_CUR);
        flags = getw(fp);
        sound = getw(fp);
        reserved_for_future[0] = getw(fp);
        reserved_for_future[1] = getw(fp);
    }
#endif
};

#define LOOPFLAG_RUNNEXTLOOP 1

struct ViewLoopNew
{
    short numFrames;
    int   flags;
    ViewFrame *frames;

    bool RunNextLoop() 
    {
        return (flags & LOOPFLAG_RUNNEXTLOOP);
    }

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

#ifdef ALLEGRO_BIG_ENDIAN
    void ReadFromFile(FILE *fp)
    {
        numloops = __getshort__bigendian(fp);
        for (int i = 0; i < 16; ++i)
        {
            numframes[i] = __getshort__bigendian(fp);
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
    }
#else
    void ReadFromFile(FILE *fp)
    {
        fread(&numloops, 2, 1, fp);
        for (int i = 0; i < 16; ++i)
        {
            fread(&numframes[i], 2, 1, fp);
        }
        fseek(fp, 2*(2 - ((16+1)%2)), SEEK_CUR);
        fread(loopflags, sizeof(int), 16, fp);
        for (int j = 0; j < 16; ++j)
        {
            for (int i = 0; i < 20; ++i)
            {
                frames[j][i].ReadFromFile(fp);
            }
        }
    }
#endif
};

#endif // __CROOM_VIEW_H