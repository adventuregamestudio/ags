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
// SpriteCache provides bitmaps by demand; it uses SpriteFile to load sprites
// and does MRU (most-recent-use) caching.
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
#ifndef __AGS_CN_AC__SPRCACHE_H
#define __AGS_CN_AC__SPRCACHE_H

#include <functional>
#include <list>
#include "core/platform.h"
#include "ac/spritefile.h"

// Tells that the sprite is found in the game resources.
#define SPRCACHEFLAG_ISASSET        0x01
// Tells that the sprite index was remapped to another existing sprite.
#define SPRCACHEFLAG_REMAPPED       0x02
// Locked sprites are ones that should not be freed when out of cache space.
#define SPRCACHEFLAG_LOCKED         0x04

// Max size of the sprite cache, in bytes
#if AGS_PLATFORM_OS_ANDROID || AGS_PLATFORM_OS_IOS
#define DEFAULTCACHESIZE_KB (32 * 1024)
#else
#define DEFAULTCACHESIZE_KB (128 * 1024)
#endif


struct SpriteInfo;

namespace AGS
{
namespace Common
{

class SpriteCache
{
public:
    static const sprkey_t MIN_SPRITE_INDEX = 1; // 0 is reserved for "empty sprite"
    static const sprkey_t MAX_SPRITE_INDEX = INT32_MAX - 1;
    static const size_t   MAX_SPRITE_SLOTS = INT32_MAX;

    typedef std::function<Size(const Size &size, const uint32_t sprite_flags)> PfnAdjustSpriteSize;
    typedef std::function<Bitmap*(sprkey_t index, Bitmap *image, uint32_t &sprite_flags)> PfnInitSprite;
    typedef std::function<void(sprkey_t index)> PfnPostInitSprite;
    typedef std::function<void(Bitmap *image)> PfnPrewriteSprite;

    struct Callbacks
    {
        PfnAdjustSpriteSize AdjustSize;
        PfnInitSprite InitSprite;
        PfnPostInitSprite PostInitSprite;
        PfnPrewriteSprite PrewriteSprite;
    };


    SpriteCache(std::vector<SpriteInfo> &sprInfos, const Callbacks &callbacks);
    ~SpriteCache();

    // Loads sprite reference information and inits sprite stream
    HError      InitFile(const Common::String &filename, const Common::String &sprindex_filename);
    // Saves current cache contents to the file
    int         SaveToFile(const Common::String &filename, int store_flags, SpriteCompression compress, SpriteFileIndex &index);
    // Closes an active sprite file stream
    void        DetachFile();

    inline int GetStoreFlags() const { return _file.GetStoreFlags(); }
    inline SpriteCompression GetSpriteCompression() const { return _file.GetSpriteCompression(); }

    // Tells if there is a sprite registered for the given index;
    // this includes sprites that were explicitly assigned but failed to init and were remapped
    bool        DoesSpriteExist(sprkey_t index) const;
    // Returns sprite's resolution; or empty Size if sprite does not exist
    Size        GetSpriteResolution(sprkey_t index) const;
    // Makes sure sprite cache has allocated slots for all sprites up to the given inclusive limit;
    // returns requested index on success, or -1 on failure.
    sprkey_t    EnlargeTo(sprkey_t topmost);
    // Finds a free slot index, if all slots are occupied enlarges sprite bank; returns index
    sprkey_t    GetFreeIndex();
    // Returns current size of the cache, in bytes; this includes locked size too!
    size_t      GetCacheSize() const;
    // Gets the total size of the locked sprites, in bytes
    size_t      GetLockedSize() const;
    // Returns maximal size limit of the cache, in bytes; this includes locked size too!
    size_t      GetMaxCacheSize() const;
    // Returns number of sprite slots in the bank (this includes both actual sprites and free slots)
    size_t      GetSpriteSlotCount() const;
    // Loads sprite and and locks in memory (so it cannot get removed implicitly)
    void        Precache(sprkey_t index);
    // Unregisters sprite from the bank and returns the bitmap
    Bitmap*     RemoveSprite(sprkey_t index);
    // Deletes particular sprite, marks slot as unused
    void        DisposeSprite(sprkey_t index);
    // Deletes all loaded (non-locked, non-external) images from the cache;
    // this keeps all the auxiliary sprite information intact
    void        DisposeAll();
    // Deletes all data and resets cache to the clear state
    void        Reset();
    // Assigns new sprite for the given index; this sprite won't be auto disposed.
    // *Deletes* the previous sprite if one was found at the same index.
    // "flags" are SPF_* constants that define sprite's behavior in game.
    bool        SetSprite(sprkey_t index, Common::Bitmap *, int flags = 0);
    // Assigns new sprite for the given index, remapping it to sprite 0;
    // optionally marks it as an asset placeholder.
    // *Deletes* the previous sprite if one was found at the same index.
    void        SetEmptySprite(sprkey_t index, bool as_asset);
    // Sets max cache size in bytes
    void        SetMaxCacheSize(size_t size);

    // Loads (if it's not in cache yet) and returns bitmap by the sprite index
    Common::Bitmap *operator[] (sprkey_t index);

private:
    // Load sprite from game resource
    size_t      LoadSprite(sprkey_t index);
    // Remap the given index to the sprite 0
    void        RemapSpriteToSprite0(sprkey_t index);
    // Gets the index of a sprite which data is used for the given slot;
    // in case of remapped sprite this will return the one given sprite is remapped to
    sprkey_t    GetDataIndex(sprkey_t index);
    // Delete the oldest (least recently used) image in cache
    void        DisposeOldest();
    // Keep disposing oldest elements until cache has at least the given free space
    void        FreeMem(size_t space);
    // Initialize the empty sprite slot
    void        InitNullSpriteParams(sprkey_t index);
    //
    // Dummy no-op variants for callbacks
    //
    static Size DummyAdjustSize(const Size &size, const uint32_t) { return size; }
    static Bitmap* DummyInitSprite(sprkey_t, Bitmap *image, uint32_t&) { return image; }
    static void DummyPostInitSprite(sprkey_t) { /* do nothing */ }
    static void DummyPrewriteSprite(Bitmap*) { /* do nothing */ }


    // Information required for the sprite streaming
    struct SpriteData
    {
        size_t          Size = 0; // to track cache size
        uint32_t        Flags = 0;
        // TODO: investigate if we may safely use unique_ptr here
        // (some of these bitmaps may be assigned from outside of the cache)
        Common::Bitmap *Image = nullptr; // actual bitmap
        // MRU list reference
        std::list<sprkey_t>::const_iterator MruIt;

        // Tells if there actually is a registered sprite in this slot
        bool DoesSpriteExist() const;
        // Tells if there's a game resource corresponding to this slot
        bool IsAssetSprite() const;
        // Tells if sprite was added externally, not loaded from game resources
        bool IsExternalSprite() const;
        // Tells if sprite is locked and should not be disposed by cache logic
        bool IsLocked() const;
    };

    // Provided map of sprite infos, to fill in loaded sprite properties
    std::vector<SpriteInfo> &_sprInfos;
    // Array of sprite references
    std::vector<SpriteData> _spriteData;

    Callbacks  _callbacks;
    SpriteFile _file;

    size_t _maxCacheSize;  // cache size limit
    size_t _lockedSize;    // size in bytes of currently locked images
    size_t _cacheSize;     // size in bytes of currently cached images

    // MRU list: the way to track which sprites were used recently.
    // When clearing up space for new sprites, cache first deletes the sprites
    // that were last time used long ago.
    std::list<sprkey_t> _mru;
};

} // namespace Common
} // namespace AGS

// Main global spritecache object
extern AGS::Common::SpriteCache spriteset;

#endif // __AGS_CN_AC__SPRCACHE_H
