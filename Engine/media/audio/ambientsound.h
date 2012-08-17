#ifndef __AC_AMBIENTSOUND_H
#define __AC_AMBIENTSOUND_H

#include "util/file.h"

#define AMBIENCE_FULL_DIST 25

struct AmbientSound {
    int  channel;  // channel number, 1 upwards
    int  x,y;
    int  vol;
    int  num;  // sound number, eg. 3 = sound3.wav
    int  maxdist;

    bool IsPlaying();

    void ReadFromFile(FILE *f);
    void WriteToFile(FILE *f);
};

#endif // __AC_AMBIENTSOUND_H
