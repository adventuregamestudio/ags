
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_lipsync.h"
#include "acmain/ac_commonheaders.h"

// lip-sync speech settings
int loops_per_character, text_lips_offset, char_speaking = -1;
char *text_lips_text = NULL;
SpeechLipSyncLine *splipsync = NULL;
int numLipLines = 0, curLipLine = -1, curLipLinePhenome = 0;

// Calculate which frame of the loop to use for this character of
// speech
int GetLipSyncFrame (char *curtex, int *stroffs) {
    /*char *frameletters[MAXLIPSYNCFRAMES] =
    {"./,/ ", "A", "O", "F/V", "D/N/G/L/R", "B/P/M",
    "Y/H/K/Q/C", "I/T/E/X/th", "U/W", "S/Z/J/ch", NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};*/

    int bestfit_len = 0, bestfit = game.default_lipsync_frame;
    for (int aa = 0; aa < MAXLIPSYNCFRAMES; aa++) {
        char *tptr = game.lipSyncFrameLetters[aa];
        while (tptr[0] != 0) {
            int lenthisbit = strlen(tptr);
            if (strchr(tptr, '/'))
                lenthisbit = strchr(tptr, '/') - tptr;

            if ((strnicmp (curtex, tptr, lenthisbit) == 0) && (lenthisbit > bestfit_len)) {
                bestfit = aa;
                bestfit_len = lenthisbit;
            }
            tptr += lenthisbit;
            while (tptr[0] == '/')
                tptr++;
        }
    }
    // If it's an unknown character, use the default frame
    if (bestfit_len == 0)
        bestfit_len = 1;
    *stroffs += bestfit_len;
    return bestfit;
}


int update_lip_sync(int talkview, int talkloop, int *talkframeptr) {
    int talkframe = talkframeptr[0];
    int talkwait = 0;

    // lip-sync speech
    char *nowsaying = &text_lips_text[text_lips_offset];
    // if it's an apostraphe, skip it (we'll, I'll, etc)
    if (nowsaying[0] == '\'') {
        text_lips_offset++;
        nowsaying++;
    }

    if (text_lips_offset >= (int)strlen(text_lips_text))
        talkframe = 0;
    else {
        talkframe = GetLipSyncFrame (nowsaying, &text_lips_offset);
        if (talkframe >= views[talkview].loops[talkloop].numFrames)
            talkframe = 0;
    }

    talkwait = loops_per_character + views[talkview].loops[talkloop].frames[talkframe].speed;

    talkframeptr[0] = talkframe;
    return talkwait;
}
