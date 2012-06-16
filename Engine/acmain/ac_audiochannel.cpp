
#include "acmain/ac_maindefines.h"


void stop_and_destroy_channel_ex(int chid, bool resetLegacyMusicSettings) {
    if ((chid < 0) || (chid > MAX_SOUND_CHANNELS))
        quit("!StopChannel: invalid channel ID");

    if (channels[chid] != NULL) {
        channels[chid]->destroy();
        delete channels[chid];
        channels[chid] = NULL;
    }

    if (play.crossfading_in_channel == chid)
        play.crossfading_in_channel = 0;
    if (play.crossfading_out_channel == chid)
        play.crossfading_out_channel = 0;

    // destroyed an ambient sound channel
    if (ambient[chid].channel > 0)
        ambient[chid].channel = 0;

    if ((chid == SCHAN_MUSIC) && (resetLegacyMusicSettings))
    {
        play.cur_music_number = -1;
        current_music_type = 0;
    }
}

void stop_and_destroy_channel (int chid) 
{
    stop_and_destroy_channel_ex(chid, true);
}
