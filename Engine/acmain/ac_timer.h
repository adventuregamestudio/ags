
#if defined(WINDOWS_VERSION)
void __cdecl dj_timer_handler();
#elif defined(LINUX_VERSION) || defined(MAC_VERSION)
extern "C" void dj_timer_handler();
#else
void dj_timer_handler(...);
#endif

int IsTimerExpired(int tnum);

extern volatile int timerloop;
extern int time_between_timers;  // in milliseconds

extern unsigned long loopcounter,lastcounter;
extern volatile unsigned long globalTimerCounter;