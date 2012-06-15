#ifndef __AC_SOUNDDEFINES_H
#define __AC_SOUNDDEFINES_H

#ifndef PSP_NO_MOD_PLAYBACK
#if (defined(LINUX_VERSION) || defined(WINDOWS_VERSION) || defined(MAC_VERSION))
#define DUMB_MOD_PLAYER
#else
#define JGMOD_MOD_PLAYER
#endif
#endif


#endif