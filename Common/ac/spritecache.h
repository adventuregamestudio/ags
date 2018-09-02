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

// TODO: research old version differences
enum SpriteFileVersion
{
    kSprfVersion_Uncompressed = 4,
    kSprfVersion_Compressed = 5,
    kSprfVersion_Last32bit = 6,
    kSprfVersion_64bit = 10,
    kSprfVersion_HighSpriteLimit = 11,
    kSprfVersion_Current = kSprfVersion_HighSpriteLimit
};

enum SpriteIndexFileVersion
{
    kSpridxfVersion_Initial = 1,
    kSpridxfVersion_Last32bit = 2,
    kSpridxfVersion_64bit = 10,
    kSpridxfVersion_HighSpriteLimit = 11,
    kSpridxfVersion_Current = kSpridxfVersion_HighSpriteLimit
};


typedef int32_t sprkey_t;

class SpriteCache
{
public:
    const sprkey_t MIN_SPRITE_INDEX = 1; // 0 is reserved for "empty sprite"
    const sprkey_t MAX_SPRITE_INDEX = INT32_MAX - 1;

    SpriteCache(sprkey_t reserve_count, std::vector<SpriteInfo> &sprInfos);

    // Tells if there is a sprite registered for the given index
    bool        DoesSpriteExist(sprkey_t index) const;
    // Makes sure sprite cache has registered slots for all sprites up to the given exclusive limit
    sprkey_t    EnlargeTo(sprkey_t newsize);
    // Finds a free slot index, if all slots are occupied enlarges sprite bank; returns index
    sprkey_t    AddNewSprite();
    // Assigns bitmap to the given slot and locks it to prevent release from cache
    void        SetSpriteAndLock(sprkey_t index, Common::Bitmap *);
    // Returns current size of the cache, in bytes
    size_t      GetCacheSize() const;
    // Gets the total size of the locked sprites, in bytes
    size_t      GetLockedSize() const;
    // Returns maximal size limit of the cache, in bytes
    size_t      GetMaxCacheSize() const;
    // Returns number of sprite slots in the bank (this includes both actual sprites and free slots)
    sprkey_t    GetSpriteSlotCount() const;
    // Finds the topmost occupied slot index. Warning: may be slow.
    sprkey_t    FindTopmostSprite() const;
    // Loads sprite and and locks in memory (so it cannot get removed implicitly)
    void        Precache(sprkey_t index);
    // Unregisters sprite from the bank and optionally deletes bitmap
    void        RemoveSprite(sprkey_t index, bool freeMemory);
    // Removes all loaded images from the cache
    void        RemoveAll();
    // Deletes all data and resets cache to the clear state
    void        Reset();
    // Assigns new bitmap for the given sprite index
    void        Set(sprkey_t index, Common::Bitmap *);
    // Sets max cache size in bytes
    void        SetMaxCacheSize(size_t size);

    // Loads sprite reference information and inits sprite stream
    int         InitFile(const char *filename);
    // Tells if bitmaps in the file are compressed
    bool        IsFileCompressed() const;
    // Opens file stream
    int         AttachFile(const char *filename);
    // Closes file stream
    void        DetachFile();
    // Saves all sprites until lastElement (exclusive) to file 
    int         SaveToFile(const char *filename, bool compressOutput);
    // Saves sprite index table in a separate file
    int         SaveSpriteIndex(const char *filename, int spriteFileIDCheck, sprkey_t lastslot, sprkey_t numsprits,
        const std::vector<int16_t> &spritewidths, const std::vector<int16_t> &spriteheights, const std::vector<soff_t> &spriteoffs);

    // Loads (if it's not in cache yet) and returns bitmap by the sprite index
    Common::Bitmap *operator[] (sprkey_t index);

private:
    void        Init(sprkey_t reserve_count = 1);
    size_t      LoadSprite(sprkey_t index);
    void        SeekToSprite(sprkey_t index);
    void        RemoveOldest();

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

    // Loads sprite index file
    bool        LoadSpriteIndexFile(int expectedFileID, soff_t spr_initial_offs, sprkey_t topmost);
    // Rebuilds sprite index from the main sprite file
    int         RebuildSpriteIndex(AGS::Common::Stream *in, sprkey_t topmost, SpriteFileVersion vers);
    // Writes compressed sprite to the stream
    void        CompressSprite(Common::Bitmap *sprite, Common::Stream *out);

    void initFile_initNullSpriteParams(sprkey_t index);
};

extern SpriteCache spriteset;

#endif // __SPRCACHE_H
