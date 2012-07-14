
#include "ac/global_timer.h"
#include "ac/rundefines.h"
#include "ac/common.h"
#include "ac/gamestate.h"

extern GameState play;


void script_SetTimer(int tnum,int timeout) {
    if ((tnum < 1) || (tnum >= MAX_TIMERS))
        quit("!StartTimer: invalid timer number");
    play.script_timers[tnum] = timeout;
}

int IsTimerExpired(int tnum) {
    if ((tnum < 1) || (tnum >= MAX_TIMERS))
        quit("!IsTimerExpired: invalid timer number");
    if (play.script_timers[tnum] == 1) {
        play.script_timers[tnum] = 0;
        return 1;
    }
    return 0;
}
