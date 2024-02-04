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
// Sprite caching system.
//
// SpriteCache provides bitmaps by demand; it uses SpriteFile to load sprites
// and does MRU (most-recent-use) caching.
//
// TODO: refactor engine code to allow store and return shared_ptr<Bitmap>.
//
// TODO: currently inherits ResourceCache<Bitmap> as protected, because sprites
// are supposed to be added through specific methods that also imply streaming
// from the file. Possibly we need another (base) class concept, something
// called a ResourceManager, for instance. ResourceManager would contain both
// some kind of a streaming object to load resources from, and a MRU Cache.
//
// TODO: there's still an inconsistency in how the class works, because it
// combines functionality of streaming "static" sprites by request from the
// game assets, and one of a "sprite list builder" used in the Editor. Latter
// allows to freely add remove and replace sprites with "asset" flag.
// This may be resolved, probably, by further separating a SpriteCache class
// into some kind of a "spritelist builder" and "runtime sprite manager".
//
//=============================================================================
#ifndef __AGS_CN_AC__SPRCACHE_H
#define __AGS_CN_AC__SPRCACHE_H

#include <functional>
#include <list>
#include <memory>
#include <vector>
#include "core/platform.h"
#include "ac/spritefile.h"
#include "gfx/bitmap.h"
#include "util/resourcecache.h"

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

class SpriteCache :
    protected ResourceCache<sprkey_t, std::unique_ptr<Bitmap>>
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
    ~SpriteCache() = default;

    // Loads sprite reference information and inits sprite stream
    HError      InitFile(const String &filename, const String &sprindex_filename);
    // Saves current cache contents to the file
    int         SaveToFile(const String &filename, int store_flags, SpriteCompression compress, SpriteFileIndex &index);
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
    inline size_t GetCacheSize() const { return ResourceCache::GetCacheSize(); }
    // Gets the total size of the locked sprites, in bytes
    inline size_t GetLockedSize() const { return ResourceCache::GetLockedSize(); }
    // Gets the total size of the external locked sprites, in bytes
    inline size_t GetExternalSize() const { return ResourceCache::GetExternalSize(); }
    // Returns maximal size limit of the cache, in bytes; this includes locked size too!
    inline size_t GetMaxCacheSize() const { return ResourceCache::GetMaxCacheSize(); }
    // Returns number of sprite slots in the bank (this includes both actual sprites and free slots)
    size_t      GetSpriteSlotCount() const;
    // Tells if the sprite storage still has unoccupied slots to put new sprites in
    bool        HasFreeSlots() const;
    // Tells if the given slot is reserved for the asset sprite, that is a "static"
    // sprite cached from the game assets
    bool        IsAssetSprite(sprkey_t index) const;
    // Tells if the sprite is loaded into the memory (either from asset file, or assigned directly)
    bool        IsSpriteLoaded(sprkey_t index) const;
    // Loads sprite using SpriteFile if such index is known,
    // frees the space if cache size reaches the limit
    void        PrecacheSprite(sprkey_t index);
    // Loads the sprite if necessary and returns a *copy* of bitmap, passing
    // ownership to the caller. Skips storing the sprite in the cache
    // (unless it was already there).
    // TODO: consider redesigning this function later; maybe there should be
    // separate sprite loader and sprite cache classes; also maybe use shared_ptrs?
    std::unique_ptr<Bitmap> LoadSpriteNoCache(sprkey_t index);
    // Locks sprite, preventing it from getting removed by the normal cache limit.
    // If this is a registered sprite from the game assets, then loads it first.
    // If this is a sprite with SPRCACHEFLAG_EXTERNAL flag, then does nothing,
    // as these are always "locked".
    // If such sprite does not exist, then fails silently.
    void        LockSprite(sprkey_t index);
    // Unlocks sprite, putting it back into the cache logic,
    // where it counts towards normal limit may be deleted to free space.
    // NOTE: sprites with SPRCACHEFLAG_EXTERNAL flag cannot be unlocked,
    // only explicitly removed.
    // If such sprite was not present in memory, then fails silently.
    void        UnlockSprite(sprkey_t index);
    // Unregisters sprite from the bank and returns the bitmap
    std::unique_ptr<Bitmap> RemoveSprite(sprkey_t index);
    // Deletes particular sprite, marks slot as unused
    void        DisposeSprite(sprkey_t index);
    // Deletes all loaded asset (non-locked, non-external) images from the cache;
    // this keeps all the auxiliary sprite information intact
    void        DisposeAllCached();
    // Deletes all data and resets cache to the clear state
    void        Reset();
    // Assigns new sprite for the given index; this sprite won't be auto disposed.
    // *Deletes* the previous sprite if one was found at the same index.
    // "flags" are optional SPF_* constants that define sprite's behavior in game.
    bool        SetSprite(sprkey_t index, std::unique_ptr<Bitmap> image, int flags = 0);
    // Assigns new dummy sprite for the given index, silently remapping it to placeholder;
    // optionally marks it as an asset placeholder.
    // *Deletes* the previous sprite if one was found at the same index.
    void        SetEmptySprite(sprkey_t index, bool as_asset);
    // Sets max cache size in bytes
    inline void SetMaxCacheSize(size_t size) { ResourceCache::SetMaxCacheSize(size); }

    // Loads (if it's not in cache yet) and returns bitmap by the sprite index
    Bitmap *operator[] (sprkey_t index);

protected:
    // Calculates item size; expects to return 0 if an item is invalid
    // and should not be added to the cache.
    size_t CalcSize(const std::unique_ptr<Bitmap> &item) override;

private:
    // Load sprite from game resource and put into the cache
    Bitmap *    LoadSprite(sprkey_t index, bool lock = false);
    // Remap the given index to the sprite 0
    void        RemapSpriteToPlaceholder(sprkey_t index);
    // Initialize the empty sprite slot
    void        InitNullSprite(sprkey_t index);
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
        uint32_t Flags = 0u;  // SPRCACHEFLAG* flags

        SpriteData() = default;

        // Tells if this slot has a valid sprite assigned (not empty slot)
        bool IsValid() const { return Flags != 0u; }
        // Tells if there's a game resource corresponding to this slot
        bool IsAssetSprite() const;
        // Tells if a sprite failed to load from assets, and should not be used
        bool IsError() const;
        // Tells if sprite was added externally, not loaded from game resources
        bool IsExternalSprite() const;
        // Tells if sprite is locked and should not be disposed by cache logic
        bool IsLocked() const;
    };

    // Provided map of sprite infos, to fill in loaded sprite properties
    std::vector<SpriteInfo> &_sprInfos;
    // Array of sprite references
    std::vector<SpriteData> _spriteData;
    // Placeholder sprite, returned from operator[] for a non-existing sprite
    std::unique_ptr<Bitmap> _placeholder;

    Callbacks  _callbacks;
    SpriteFile _file;
};

} // namespace Common
} // namespace AGS

// Main global spritecache object
extern AGS::Common::SpriteCache spriteset;

#endif // __AGS_CN_AC__SPRCACHE_H
