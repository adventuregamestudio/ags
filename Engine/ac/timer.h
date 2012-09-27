
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__TIMER_H
#define __AGS_EE_AC__TIMER_H

#if defined(WINDOWS_VERSION)
void __cdecl dj_timer_handler();
#elif defined (DOS_VERSION)
void dj_timer_handler(...);
#else
extern "C" void dj_timer_handler();
#endif

#endif // __AGS_EE_AC__TIMER_H
