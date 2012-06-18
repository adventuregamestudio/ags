
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_timer.h"
#include "ac/ac_common.h"
#include "acrun/ac_runninggame.h"

extern volatile int mvolcounter; // in ac_music

unsigned long loopcounter=0,lastcounter=0;
volatile unsigned long globalTimerCounter = 0;

volatile int timerloop=0;
int time_between_timers=25;  // in milliseconds
// our timer, used to keep game running at same speed on all systems
#if defined(WINDOWS_VERSION)
void __cdecl dj_timer_handler() {
#elif defined(LINUX_VERSION) || defined(MAC_VERSION)
extern "C" void dj_timer_handler() {
#else
void dj_timer_handler(...) {
#endif
    timerloop++;
    globalTimerCounter++;
    if (mvolcounter > 0) mvolcounter++;
}
END_OF_FUNCTION(dj_timer_handler);


int IsTimerExpired(int tnum) {
  if ((tnum < 1) || (tnum >= MAX_TIMERS))
    quit("!IsTimerExpired: invalid timer number");
  if (play.script_timers[tnum] == 1) {
    play.script_timers[tnum] = 0;
    return 1;
    }
  return 0;
  }
