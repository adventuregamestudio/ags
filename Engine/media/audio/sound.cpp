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

SOUNDCLIP *my_load_wave(const AssetPath &asset_name, bool loop)
{
    return my_load_openal(asset_name, "WAV", loop);
}

SOUNDCLIP *my_load_mp3(const AssetPath &asset_name, bool loop)
{
    return my_load_openal(asset_name, "MP3", loop);
}

SOUNDCLIP *my_load_ogg(const AssetPath &asset_name, bool loop)
{
    return my_load_openal(asset_name, "OGG", loop);
}

SOUNDCLIP *my_load_midi(const AssetPath &asset_name, bool loop)
{
    return my_load_openal(asset_name, "MIDI", loop);
}

SOUNDCLIP *my_load_mod(const AssetPath &asset_name, bool loop)
{
    return my_load_openal(asset_name, "MOD", loop);
}

