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
#include <cmath>
#include <list>
#include <unordered_map>
#include "core/assetmanager.h"
#include "media/audio/audio_core.h"
#include "media/audio/audiodefines.h"
#include "util/path.h"
#include "util/stream.h"
#include "util/string_types.h"

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

// Sound cache, stores most recent used sounds, tracks use history with MRU list.
// TODO: refactor into the resource cache class, share with the sprite cache.
class SoundCache
{
public:
    typedef std::shared_ptr<std::vector<uint8_t>> DataRef;

    void SetMaxCacheSize(size_t size)
    {
        _maxSize = size;
        FreeMem(0); // makes sure it does not exceed max size
    }

    DataRef Get(const String &name)
    {
        const auto found = _map.find(name);
        if (found == _map.end())
            return DataRef();
        // Move to the beginning of the MRU list
        _mru.splice(_mru.begin(), _mru, found->second.MruIt);
        return found->second.Data;
    }

    // Add particular item into the cache, remove oldest items if cache size is exceeded
    void Put(const String &name, DataRef &ref)
    {
        assert(ref);
        if (!ref)
            return; // safety precaution
        if (Get(name))
            return; // already in cache
        if (_maxSize == 0)
            return; // cache is disabled
        // Clear up space before adding
        if (_cacheSize + ref->size() > _maxSize)
            FreeMem(ref->size());
        SoundEntry entry(name, ref);
        entry.MruIt = _mru.insert(_mru.begin(), name);
        _map[name] = std::move(entry);
        _cacheSize += ref->size();
    }

    // Clear the cache, dispose all resources
    void Clear()
    {
        _map.clear();
        _mru.clear();
        _cacheSize = 0u;
    }

private:
    // Delete the oldest (least recently used) item in cache
    void DisposeOldest()
    {
        assert(_mru.size() > 0);
        if (_mru.size() == 0) return;
        auto it = std::prev(_mru.end());
        const auto id = *it;
        _cacheSize -= _map[id].Data->size();
        _map.erase(id);
        // Remove from the mru list
        _mru.erase(it);
    }
    // Keep disposing oldest elements until cache has at least the given free space
    void FreeMem(size_t space)
    {
        while ((_mru.size() > 0) && (_cacheSize >= (_maxSize - space)))
        {
            DisposeOldest();
        }
    }

    struct SoundEntry
    {
        String Name;
        DataRef Data;
        // MRU list reference
        std::list<String>::const_iterator MruIt;
        SoundEntry() = default;
        SoundEntry(const String &name, DataRef &data)
            : Name(name), Data(data) {}
    };

    size_t _cacheSize = 0u;
    size_t _maxSize = DEFAULT_SOUNDCACHESIZE_KB;
    std::unordered_map<String, SoundEntry, HashStrNoCase> _map;
    // MRU list: the way to track which items were used recently.
    // When clearing up space for new items, cache first deletes the items
    // that were last time used long ago.
    std::list<String> _mru;
};


// Maximal sound asset size which is allowed to be loaded at once;
// anything larger will be streamed
// TODO: make configureable?
static size_t MaxLoadAtOnce = DEFAULT_SOUNDLOADATONCE_KB;
static SoundCache SndCache;

void soundcache_set_rules(size_t max_loadatonce, size_t max_cachesize)
{
    MaxLoadAtOnce = max_loadatonce;
    SndCache.SetMaxCacheSize(max_cachesize);
}

void soundcache_clear()
{
    SndCache.Clear();
}

static SOUNDCLIP *my_load_clip(const AssetPath &apath, const char *extension_hint, bool loop)
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
        s_in.reset(AssetMgr->OpenAsset(apath));
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
