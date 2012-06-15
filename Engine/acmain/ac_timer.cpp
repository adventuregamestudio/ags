
#include "acmain/ac_maindefines.h"


int IsTimerExpired(int tnum) {
  if ((tnum < 1) || (tnum >= MAX_TIMERS))
    quit("!IsTimerExpired: invalid timer number");
  if (play.script_timers[tnum] == 1) {
    play.script_timers[tnum] = 0;
    return 1;
    }
  return 0;
  }
