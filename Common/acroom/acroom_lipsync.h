#ifndef __CROOM_LIPSYNC_H
#define __CROOM_LIPSYNC_H

struct SpeechLipSyncLine {
    char  filename[14];
    int  *endtimeoffs;
    short*frame;
    short numPhenomes;
};

#endif // __CROOM_LIPSYNC_H