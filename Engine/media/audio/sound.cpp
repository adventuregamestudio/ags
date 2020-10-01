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
#include "media/audio/sound.h"
#include "media/audio/clip_openal.h"

SOUNDCLIP *my_load_wave(const AssetPath &asset_name, int voll, int loop)
{
    return my_load_openal(asset_name, "WAV", voll, loop);
}

SOUNDCLIP *my_load_mp3(const AssetPath &asset_name, int voll)
{
    return my_load_openal(asset_name, "MP3", voll, false);
}

SOUNDCLIP *my_load_static_mp3(const AssetPath &asset_name, int voll, bool loop)
{
    return my_load_openal(asset_name, "MP3", voll, loop);
}

SOUNDCLIP *my_load_static_ogg(const AssetPath &asset_name, int voll, bool loop)
{
    return my_load_openal(asset_name, "OGG", voll, loop);
}

SOUNDCLIP *my_load_ogg(const AssetPath &asset_name, int voll)
{
    return my_load_openal(asset_name, "OGG", voll, false);
}

SOUNDCLIP *my_load_midi(const AssetPath &asset_name, int repet)
{
    return my_load_openal(asset_name, "MIDI", 0, repet);
}

SOUNDCLIP *my_load_mod(const AssetPath &asset_name, int repet)
{
    return my_load_openal(asset_name, "MOD", 0, repet);
}

