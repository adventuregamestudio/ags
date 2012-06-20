#ifndef __AC_LIPSYNC_H
#define __AC_LIPSYNC_H

struct SpeechLipSyncLine {
    char  filename[14];
    int  *endtimeoffs;
    short*frame;
    short numPhenomes;
};

#endif // __AC_LIPSYNC_H