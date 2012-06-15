#ifndef __AC_SOUNDINTERNALDEFS_H
#define __AC_SOUNDINTERNALDEFS_H

#if ALLEGRO_DATE > 20050101
// because we have to use Allegro 4.2
// and the packfile format has changed slightly
// 'todo' has been put in a structure called 'normal'
#define todo normal.todo
#endif

#if defined(ANDROID_VERSION) && defined(SOUND_CACHE_DEBUG)
extern "C" void android_debug_printf(char* format, ...);
#define printf android_debug_printf
#endif


//#define MP3CHUNKSIZE 100000
#define MP3CHUNKSIZE 32768

#endif // __AC_SOUNDINTERNALDEFS_H