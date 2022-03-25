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
#include <cmath>
#include "core/assetmanager.h"
#include "media/audio/audio_core.h"
#include "media/audio/audiodefines.h"
#include "util/path.h"
#include "util/stream.h"

using namespace AGS::Common;

static int GuessSoundTypeFromExt(const String &extension)
{
    if (extension.CompareNoCase("mp3") == 0) {
        return MUS_MP3;
    }
    else if (extension.CompareNoCase("ogg") == 0) {
        return MUS_OGG;
    }
    else if (extension.CompareNoCase("mid") == 0) {
        return MUS_MIDI;
    }
    else if (extension.CompareNoCase("wav") == 0) {
        return MUS_WAVE;
    }
    else if (extension.CompareNoCase("voc") == 0) {
        return MUS_WAVE;
    }
    else if (extension.CompareNoCase("mod") == 0) {
        return MUS_MOD;
    }
    else if (extension.CompareNoCase("s3m") == 0) {
        return MUS_MOD;
    }
    else if (extension.CompareNoCase("it") == 0) {
        return MUS_MOD;
    }
    else if (extension.CompareNoCase("xm") == 0) {
        return MUS_MOD;
    }
    return 0;
}

static SOUNDCLIP *my_load_clip(const AssetPath &apath, const char *extension_hint, bool loop)
{
    auto *s = AssetMgr->OpenAsset(apath);
    if (!s)
        return nullptr;

    const size_t asset_size = static_cast<size_t>(s->GetLength());
    std::vector<char> data(asset_size);
    s->Read(data.data(), asset_size);
    delete s;

    const auto asset_ext = AGS::Common::Path::GetFileExtension(apath.Name);
    const auto ext_hint = asset_ext.IsEmpty() ? String(extension_hint) : asset_ext;

    auto slot = audio_core_slot_init(data, ext_hint, loop);
    if (slot < 0) { return nullptr; }

    const auto sound_type = GuessSoundTypeFromExt(ext_hint);
    const auto lengthMs = (int)std::round(audio_core_slot_get_duration(slot));

    auto clip = new SOUNDCLIP(slot);
    clip->repeat = loop;
    clip->soundType = sound_type;
    clip->lengthMs = lengthMs;
    return clip;
}

SOUNDCLIP *my_load_wave(const AssetPath &asset_name, bool loop)
{
    return my_load_clip(asset_name, "WAV", loop);
}

SOUNDCLIP *my_load_mp3(const AssetPath &asset_name, bool loop)
{
    return my_load_clip(asset_name, "MP3", loop);
}

SOUNDCLIP *my_load_ogg(const AssetPath &asset_name, bool loop)
{
    return my_load_clip(asset_name, "OGG", loop);
}

SOUNDCLIP *my_load_midi(const AssetPath &asset_name, bool loop)
{
    return my_load_clip(asset_name, "MIDI", loop);
}

SOUNDCLIP *my_load_mod(const AssetPath &asset_name, bool loop)
{
    return my_load_clip(asset_name, "MOD", loop);
}
