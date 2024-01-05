//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// SOUNDCLIP factory methods.
//
//=============================================================================
#ifndef __AC_SOUND_H
#define __AC_SOUND_H

#include "ac/asset_helper.h"
#include "media/audio/soundclip.h"

// Threshold in bytes for loading sounds immediately, in KB
const size_t DEFAULT_SOUNDLOADATONCE_KB = 1024u;
// Sound cache limit, in KB
const size_t DEFAULT_SOUNDCACHESIZE_KB = 1024u * 32; // 32 MB

// Sets sound loading and caching rules:
// * max_loadatonce - threshold in bytes for loading sounds immediately, vs streaming
// * max_cachesize - sound cache limit, in bytes
void soundcache_set_rules(size_t max_loadatonce, size_t max_cachesize);
void soundcache_clear();
void soundcache_precache(const AssetPath &apath);

SOUNDCLIP *load_sound_clip(const AssetPath &apath, const char *extension_hint, bool loop);

#endif // __AC_SOUND_H
