
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__TIMER_H
#define __AGS_EE_AC__TIMER_H

#if defined(WINDOWS_VERSION)
void __cdecl dj_timer_handler();
#elif defined(LINUX_VERSION) || defined(MAC_VERSION)
extern "C" void dj_timer_handler();
#else
void dj_timer_handler(...);
#endif

#endif // __AGS_EE_AC__TIMER_H
