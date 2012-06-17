#ifndef __AC_AMBIENTSOUND_H
#define __AC_AMBIENTSOUND_H

struct AmbientSound {
    int  channel;  // channel number, 1 upwards
    int  x,y;
    int  vol;
    int  num;  // sound number, eg. 3 = sound3.wav
    int  maxdist;

    bool IsPlaying();
};

#endif // __AC_AMBIENTSOUND_H
