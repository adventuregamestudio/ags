//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

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