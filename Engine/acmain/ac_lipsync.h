
#include "ac/lipsync.h"

int update_lip_sync(int talkview, int talkloop, int *talkframeptr);

// lip-sync speech settings
extern int loops_per_character, text_lips_offset, char_speaking;
extern char *text_lips_text;
extern SpeechLipSyncLine *splipsync;
extern int numLipLines, curLipLine, curLipLinePhenome;
