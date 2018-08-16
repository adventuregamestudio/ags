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
//
// Sprite caching system.
//
// TODO: split out sprite serialization into something like "SpriteFile" class.
// SpriteCache should only manage caching and provide bitmaps by demand.
// There should be a separate unit which manages streaming and decompressing.
// Perhaps an interface which would allow multiple implementation depending
// on compression type.
//
// TODO: use 64-bit file offsets for over 2GB files.
//
// TODO: store sprite data in a specialized container type that is optimized
// for having most keys allocated in large continious sequences by default.
//
// Only for the reference: one of the ideas is for container to have a table
// of arrays of fixed size internally. When getting an item the hash would be
// first divided on array size to find the array the item resides in, then the
// item is taken from item from slot index = (hash - arrsize * arrindex).
// TODO: find out if there is already a hash table kind that follows similar
// principle.
//
//=============================================================================

#ifndef __SPRCACHE_H
#define __SPRCACHE_H

#include <vector>
#include "ac/gamestructdefines.h"

namespace AGS { namespace Common { class Stream; class Bitmap; } }
using namespace AGS; // FIXME later

// We can't rely on offsets[slot]==0 because when the engine is running
// this is changed to reference the Bluecup sprite. Therefore we need
// a definite way of knowing whether the sprite existed in the sprite file.
#define SPRCACHEFLAG_DOESNOTEXIST   0x01
// Locked sprites are ones that should not be freed when clearing cache space.
#define SPRCACHEFLAG_LOCKED         0x02

// Max size of the sprite cache, in bytes
#if defined (PSP_VERSION)
// PSP: Use smaller sprite cache due to limited total memory.
#define DEFAULTCACHESIZE 5 * 1024 * 1024
#elif defined (ANDROID_VERSION) || defined (IOS_VERSION)
#define DEFAULTCACHESIZE 32 * 1024 * 1024
#else
#define DEFAULTCACHESIZE 128 * 1024 * 1024
#endif

typedef int32_t sprkey_t;

class SpriteCache
{
public:
    const sprkey_t MIN_SPRITE_INDEX = 1; // 0 is reserved for "empty sprite"
    const sprkey_t MAX_SPRITE_INDEX = INT32_MAX - 1;

    SpriteCache(sprkey_t reserve_count, std::vector<SpriteInfo> &sprInfos);

    // Tells if there is a sprite registered for the given index
    bool        DoesSpriteExist(sprkey_t index);
    // Makes sure sprite cache has registered slots for all sprites up to the given exclusive limit
    sprkey_t    EnlargeTo(sprkey_t newsize);
    // Finds a free slot index, if all slots are occupied enlarges sprite bank; returns index
    sprkey_t    AddNewSprite();
    // Returns current size of the cache, in bytes
    size_t      GetCacheSize() const;
    // Gets the total size of the locked sprites, in bytes
    size_t      GetLockedSize() const;
    // Returns maximal size limit of the cache, in bytes
    size_t      GetMaxCacheSize() const;
    // Returns number of sprite slots in the bank (this includes both actual sprites and free slots)
    sprkey_t    GetSpriteSlotCount() const;
    // Loads sprite reference information and inits sprite stream
    int         InitFile(const char *);
    // Loads sprite and and locks in memory (so it cannot get removed implicitly)
    void        Precache(sprkey_t index);
    // Removes all loaded images from the cache
    void        RemoveAll();
    // Deletes all data and resets cache to the clear state
    void        Reset();
    // Assigns new bitmap for the given sprite index
    void        Set(sprkey_t index, Common::Bitmap *);
    // Sets max cache size in bytes
    void        SetMaxCacheSize(size_t size);

    // Loads (if it's not in cache yet) and returns bitmap by the sprite index
    Common::Bitmap *operator[] (sprkey_t index);

private:
    size_t loadSprite(sprkey_t);
    void seekToSprite(sprkey_t index);
    void setNonDiscardable(sprkey_t, Common::Bitmap *);
    void removeSprite(sprkey_t, bool);
    void removeOldest();
    void init(sprkey_t reserve_count = 1);
    int  saveToFile(const char *, sprkey_t lastElement, bool compressOutput);
    void detachFile();
    int  attachFile(const char *);

    // Information required for the sprite streaming
    // TODO: make compatible with large (over 2GB) files
    // (may need some data conversion when loading sprite index file)
    // TODO: split into sprite cache and sprite stream data
    struct SpriteData
    {
        soff_t          Offset; // data offset
        soff_t          Size;   // cache size of element, in bytes
        uint8_t         Flags;
        // TODO: investigate if we may safely use unique_ptr here
        // (some of these bitmaps may be assigned from outside of the cache)
        Common::Bitmap *Image; // actual bitmap

        SpriteData();
        ~SpriteData();
    };

    // Provided map of sprite infos, to fill in loaded sprite properties
    std::vector<SpriteInfo> &_sprInfos;
    // Array of sprite references
    std::vector<SpriteData> _spriteData;
    bool _compressed;        // are sprites compressed
    soff_t _sprite0InitialOffset; // offset of the first sprite in the stream

    Common::Stream *_stream; // the sprite stream
    sprkey_t _lastLoad; // last loaded sprite index

    size_t _maxCacheSize;  // cache size limit
    size_t _lockedSize;    // size in bytes of currently locked images
    size_t _cacheSize;     // size in bytes of currently cached images

    // MRU list: the way to track which sprites were used recently.
    // When clearing up space for new sprites, cache first deletes the sprites
    // that were last time used long ago.
    std::vector<sprkey_t> _mrulist;
    std::vector<sprkey_t> _mrubacklink;
    int _liststart;
    int _listend;

    void compressSprite(Common::Bitmap *sprite, Common::Stream *out);
    bool loadSpriteIndexFile(int expectedFileID, soff_t spr_initial_offs, sprkey_t numspri);

    void initFile_adjustBuffers(sprkey_t numspri);
    void initFile_initNullSpriteParams(sprkey_t index);
};

extern SpriteCache spriteset;

#endif // __SPRCACHE_H
