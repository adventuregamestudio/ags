
#include "wgt2allg.h"
#include "ac/timer.h"

extern volatile int mvolcounter;

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
