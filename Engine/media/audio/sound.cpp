//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "media/audio/sound.h"
#include <list>
#include <unordered_map>
#include "ac/game.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "media/audio/audio_core.h"
#include "media/audio/audiodefines.h"
#include "util/path.h"
#include "util/resourcecache.h"
#include "util/stream.h"
#include "util/string_types.h"

using namespace AGS::Common;
using namespace AGS::Engine;

// FIXME: merge with ScriptAudioClip::GetAudioFileTypeFromExt!
static AudioFileType GuessSoundTypeFromExt(const String &extension)
{
    if (extension.CompareNoCase("mp3") == 0) {
        return eAudioFileMP3;
    }
    else if (extension.CompareNoCase("ogg") == 0) {
        return eAudioFileOGG;
    }
    else if (extension.CompareNoCase("mid") == 0) {
        return eAudioFileMIDI;
    }
    else if (extension.CompareNoCase("wav") == 0) {
        return eAudioFileWAV;
    }
    else if (extension.CompareNoCase("voc") == 0) {
        return eAudioFileWAV;
    }
    else if (extension.CompareNoCase("mod") == 0) {
        return eAudioFileMOD;
    }
    else if (extension.CompareNoCase("s3m") == 0) {
        return eAudioFileMOD;
    }
    else if (extension.CompareNoCase("it") == 0) {
        return eAudioFileMOD;
    }
    else if (extension.CompareNoCase("xm") == 0) {
        return eAudioFileMOD;
    }
    else if (extension.CompareNoCase("flac") == 0) {
        return eAudioFileFLAC;
    }
    return eAudioFileUndefined;
}

// Sound cache, stores most recent used sounds, tracks use history with MRU list.
class SoundCache final :
    public ResourceCache<String, std::shared_ptr<std::vector<uint8_t>>>
{
public:
    typedef std::shared_ptr<std::vector<uint8_t>> DataRef;

    SoundCache() : ResourceCache(DEFAULT_SOUNDCACHESIZE_KB)
    {
    }

private:
    // Calculates item size; expects to return 0 if an item is invalid
    // and should not be added to the cache.
    size_t CalcSize(const DataRef &item) override
    {
        assert(item);
        return item ? item->size() : 0u;
    }
};


// Maximal sound asset size which is allowed to be loaded at once;
// anything larger will be streamed
static size_t MaxLoadAtOnce = DEFAULT_SOUNDLOADATONCE_KB;
static SoundCache SndCache;

void soundcache_set_rules(size_t max_loadatonce, size_t max_cachesize)
{
    MaxLoadAtOnce = max_loadatonce;
    SndCache.SetMaxCacheSize(max_cachesize);
    Debug::Printf("Sound cache set: %zu KB", max_cachesize / 1024);
}

void soundcache_clear()
{
    SndCache.Clear();
}

void soundcache_precache(const AssetPath &apath)
{
    if (SndCache.GetMaxCacheSize() == 0)
        return; // cache is disabled
    if (SndCache.Exists(apath.Name))
        return; // already in cache
    auto s_in = AssetMgr->OpenAsset(apath);
    if (!s_in)
        return; // failed to open asset
    size_t asset_size = static_cast<size_t>(s_in->GetLength());
    if (asset_size > MaxLoadAtOnce)
        return; // too big for the cache
    // Read and put into the cache
    auto sounddata = std::make_shared<std::vector<uint8_t>>(asset_size);
    s_in->Read(sounddata->data(), asset_size);
    SndCache.Put(apath.Name, sounddata);
}

std::unique_ptr<SoundClip> load_sound_clip(const AssetPath &apath, const char *extension_hint, bool loop)
{
    size_t asset_size;
    std::unique_ptr<Stream> s_in;
    auto sounddata = SndCache.Get(apath.Name);
    if (sounddata)
    {
        asset_size = sounddata->size();
    }
    else
    {
        s_in = AssetMgr->OpenAsset(apath);
        if (!s_in)
            return nullptr;
        asset_size = static_cast<size_t>(s_in->GetLength());
    }

    const auto asset_ext = AGS::Common::Path::GetFileExtension(apath.Name);
    const auto ext_hint = asset_ext.IsEmpty() ? String(extension_hint) : asset_ext;

    int slot{};
    // If sound data was cached, or asset's size is small enough to load at once,
    // then load/use it and update the cache if necessary
    if (sounddata || asset_size <= MaxLoadAtOnce)
    {
        if (!sounddata)
        {
            sounddata.reset(new std::vector<uint8_t>(asset_size));
            s_in->Read(sounddata->data(), asset_size);
            SndCache.Put(apath.Name, sounddata);
        }
        slot = audio_core_slot_init(sounddata, ext_hint, loop);
    }
    // Otherwise, if asset's size is too large, start streaming
    else
    {
        slot = audio_core_slot_init(std::move(s_in), ext_hint, loop);
    }

    if (slot < 0) { return nullptr; }

    const auto sound_type = GuessSoundTypeFromExt(ext_hint);
    return std::unique_ptr<SoundClip>(new SoundClip(slot, sound_type, loop));
}
