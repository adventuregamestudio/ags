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
// sprite caching system
//
//=============================================================================

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
#endif

#include "ac/common.h"
#include "ac/spritecache.h"
#include "core/assetmanager.h"
#include "debug/out.h"
#include "gfx/bitmap.h"
#include "util/compress.h"
#include "util/file.h"
#include "util/stream.h"

using namespace AGS::Common;

// [IKM] We have to forward-declare these because their implementations are in the Engine
extern void initialize_sprite(int);
extern void pre_save_sprite(int);
extern void get_new_size_for_sprite(int, int, int, int &, int &);

#define START_OF_LIST -1
#define END_OF_LIST   -1

const char *spindexid = "SPRINDEX";
const char *spindexfilename = "sprindex.dat";

// TODO: research old version differences
enum SpriteFileVersion
{
    kSprfVersion_Uncompressed = 4,
    kSprfVersion_Compressed = 5,
    kSprfVersion_Last32bit = 6,
    kSprfVersion_64bit = 10,
    kSprfVersion_Current = kSprfVersion_64bit
};

enum SpriteIndexFileVersion
{
    kSpridxfVersion_Initial = 1,
    kSpridxfVersion_Last32bit = 2,
    kSpridxfVersion_64bit = 10,
    kSpridxfVersion_Current = kSpridxfVersion_64bit
};



SpriteInfo::SpriteInfo()
    : Flags(0)
    , Width(0)
    , Height(0)
{
}

SpriteCache::SpriteData::SpriteData()
    : Offset(0)
    , Size(0)
    , Flags(SPRCACHEFLAG_DOESNOTEXIST)
    , Image(NULL)
{
}

SpriteCache::SpriteData::~SpriteData()
{
    // TODO: investigate, if it's actually safe/preferred to delete bitmap here
    // (some of these bitmaps may be assigned from outside of the cache)
}


SpriteCache::SpriteCache(sprkey_t reserve_count, std::vector<SpriteInfo> &sprInfos)
    : _sprInfos(sprInfos)
    , _stream(NULL)
{
    _sprite0InitialOffset = 0;
    _compressed = false;
    init(reserve_count);
}

size_t SpriteCache::GetCacheSize() const
{
    return _cacheSize;
}

size_t SpriteCache::GetLockedSize() const
{
    return _lockedSize;
}

size_t SpriteCache::GetMaxCacheSize() const
{
    return _maxCacheSize;
}

sprkey_t SpriteCache::GetSpriteSlotCount() const
{
    return _spriteData.size();
}

void SpriteCache::SetMaxCacheSize(size_t size)
{
    _maxCacheSize = size;
}

void SpriteCache::init(sprkey_t reserve_count)
{
    delete _stream;
    _stream = NULL;
    EnlargeTo(reserve_count);
    _cacheSize = 0;
    _lockedSize = 0;
    _maxCacheSize = DEFAULTCACHESIZE;
    _liststart = -1;
    _listend = -1;
    _lastLoad = -2;
}

void SpriteCache::Reset()
{
    // TODO: find out if it's safe to simply always delete _spriteData.Image with array element
    for (size_t i = 0; i < _spriteData.size(); ++i)
    {
        if (_spriteData[i].Image)
        {
            delete _spriteData[i].Image;
            _spriteData[i].Image = NULL;
        }
    }
    _spriteData.clear();

    _mrulist.clear();
    _mrubacklink.clear();

    init();
}

void SpriteCache::Set(sprkey_t index, Bitmap *sprite)
{
    EnlargeTo(index + 1);
    _spriteData[index].Image = sprite;
}

void SpriteCache::setNonDiscardable(sprkey_t index, Bitmap *sprite)
{
    EnlargeTo(index + 1);
    _spriteData[index].Image = sprite;
    _spriteData[index].Flags |= SPRCACHEFLAG_LOCKED;
}

void SpriteCache::removeSprite(sprkey_t index, bool freeMemory)
{
    if ((_spriteData[index].Image != NULL) && (freeMemory))
        delete _spriteData[index].Image;

    _spriteData[index].Image = NULL;
    _spriteData[index].Offset = 0;
}

sprkey_t SpriteCache::EnlargeTo(sprkey_t newsize)
{
    if (newsize < 0 || (size_t)newsize <= _spriteData.size())
        return 0;
    if (newsize > MAX_SPRITE_INDEX + 1)
        newsize = MAX_SPRITE_INDEX + 1;

    sprkey_t elementsWas = (sprkey_t)_spriteData.size();
    _sprInfos.resize(newsize);
    _spriteData.resize(newsize);
    _mrulist.resize(newsize);
    _mrubacklink.resize(newsize);
    return elementsWas;
}

sprkey_t SpriteCache::AddNewSprite()
{
    if (_spriteData.size() == MAX_SPRITE_INDEX + 1)
        return -1; // no more sprite allowed
    for (size_t i = MIN_SPRITE_INDEX; i < _spriteData.size(); ++i)
    {
        // slot empty
        if ((_spriteData[i].Image == NULL) && ((_spriteData[i].Offset == 0) || (_spriteData[i].Offset == _sprite0InitialOffset)))
        {
            _sprInfos[i] = SpriteInfo();
            _spriteData[i] = SpriteData();
            return i;
        }
    }
    // enlarge the sprite bank to find a free slot and return the first new free slot
    return EnlargeTo(_spriteData.size() + 1); // we use +1 here to let std container decide the actual reserve size
}

bool SpriteCache::DoesSpriteExist(sprkey_t index)
{
    if (_spriteData[index].Image != NULL)
        return true;
    if (_spriteData[index].Flags & SPRCACHEFLAG_DOESNOTEXIST)
        return false;
    if (_spriteData[index].Offset > 0)
        return true;
    return false;
}

Bitmap *SpriteCache::operator [] (sprkey_t index)
{
    // invalid sprite slot
    if (index < 0 || (size_t)index >= _spriteData.size())
        return NULL;

    // Dynamically added sprite, don't put it on the sprite list
    if ((_spriteData[index].Image != NULL) &&
        ((_spriteData[index].Offset == 0) || ((_spriteData[index].Flags & SPRCACHEFLAG_DOESNOTEXIST) != 0)))
        return _spriteData[index].Image;

    // if sprite exists in file but is not in mem, load it
    if ((_spriteData[index].Image == NULL) && (_spriteData[index].Offset > 0))
        loadSprite(index);

    // Locked sprite, eg. mouse cursor, that shouldn't be discarded
    if (_spriteData[index].Flags & SPRCACHEFLAG_LOCKED)
        return _spriteData[index].Image;

    if (_liststart < 0)
    {
        _liststart = index;
        _listend = index;
        _mrulist[index] = END_OF_LIST;
        _mrubacklink[index] = START_OF_LIST;
    } 
    else if (_listend != index)
    {
        // this is the oldest element being bumped to newest, so update start link
        if (index == _liststart)
        {
            _liststart = _mrulist[index];
            _mrubacklink[_liststart] = START_OF_LIST;
        }
        // already in list, link previous to next
        else if (_mrulist[index] > 0)
        {
            _mrulist[_mrubacklink[index]] = _mrulist[index];
            _mrubacklink[_mrulist[index]] = _mrubacklink[index];
        }

        // set this as the newest element in the list
        _mrulist[index] = END_OF_LIST;
        _mrulist[_listend] = index;
        _mrubacklink[index] = _listend;
        _listend = index;
    }

    return _spriteData[index].Image;
}

// Remove the oldest cache element
void SpriteCache::removeOldest()
{
    if (_liststart < 0)
        return;

    sprkey_t sprnum = _liststart;

    if ((_spriteData[sprnum].Image != NULL) && !(_spriteData[sprnum].Flags & SPRCACHEFLAG_LOCKED)) {
        // Free the memory
        if (_spriteData[sprnum].Flags & SPRCACHEFLAG_DOESNOTEXIST)
        {
            char msgg[150];
            sprintf(msgg, "SpriteCache::removeOldest: Attempted to remove sprite %d that does not exist", sprnum);
            quit(msgg);
        }
        _cacheSize -= _spriteData[sprnum].Size;

        delete _spriteData[sprnum].Image;
        _spriteData[sprnum].Image = NULL;
    }

    if (_liststart == _listend)
    {
        // there was one huge sprite, removing it has now emptied the cache completely
        if (_cacheSize > 0)
        {
            Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "SPRITE CACHE ERROR: Sprite cache should be empty, but still has %d bytes", _cacheSize);
        }
        _mrulist[_liststart] = 0;
        _liststart = -1;
        _listend = -1;
    }
    else
    {
        sprkey_t oldstart = _liststart;
        _liststart = _mrulist[_liststart];
        _mrulist[oldstart] = 0;
        _mrubacklink[_liststart] = START_OF_LIST;
        if (oldstart == _liststart)
        {
            // Somehow, we have got a recursive link to itself, so we
            // the game will freeze (since it is not actually freeing any
            // memory)
            // There must be a bug somewhere causing this, but for now
            // let's just reset the cache
            Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "RUNTIME CACHE ERROR: CACHE INCONSISTENT: RESETTING\n\tAt size %d (of %d), start %d end %d  fwdlink=%d",
                        _cacheSize, _maxCacheSize, oldstart, _listend, _liststart);
            RemoveAll();
        }
    }

#ifdef DEBUG_SPRITECACHE
    Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Debug, "Removed %d, size now %d KB", sprnum, _cacheSize / 1024);
#endif
}

void SpriteCache::RemoveAll()
{
    _liststart = -1;
    _listend = -1;
    for (size_t i = 0; i < _spriteData.size(); ++i)
    {
        if (!(_spriteData[i].Flags & SPRCACHEFLAG_LOCKED) && (_spriteData[i].Image != NULL) &&
            ((_spriteData[i].Flags & SPRCACHEFLAG_DOESNOTEXIST) == 0))
        {
            delete _spriteData[i].Image;
            _spriteData[i].Image = NULL;
        }
        _mrulist[i] = 0;
        _mrubacklink[i] = 0;
    }
    _cacheSize = _lockedSize;
}

void SpriteCache::Precache(sprkey_t index)
{
    if (index < 0 || (size_t)index >= _spriteData.size())
        return;

    soff_t sprSize = 0;

    if (_spriteData[index].Image == NULL)
        sprSize = loadSprite(index);
    else if (!(_spriteData[index].Flags & SPRCACHEFLAG_LOCKED))
        sprSize = _spriteData[index].Size;

    // make sure locked sprites can't fill the cache
    _maxCacheSize += sprSize;
    _lockedSize += sprSize;

    _spriteData[index].Flags |= SPRCACHEFLAG_LOCKED;

#ifdef DEBUG_SPRITECACHE
    Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Debug, "Precached %d", index);
#endif
}

void SpriteCache::seekToSprite(sprkey_t index)
{
    if (index - 1 != _lastLoad)
        _stream->Seek(_spriteData[index].Offset, kSeekBegin);
}

size_t SpriteCache::loadSprite(sprkey_t index)
{
    int hh = 0;

    while (_cacheSize > _maxCacheSize)
    {
        removeOldest();
        hh++;
        if (hh > 1000)
        {
            Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "RUNTIME CACHE ERROR: STUCK IN FREE_UP_MEM; RESETTING CACHE");
            RemoveAll();
        }
    }

    if (index < 0 || (size_t)index >= _spriteData.size())
        quit("sprite cache array index out of bounds");

    // If we didn't just load the previous sprite, seek to it
    seekToSprite(index);

    int coldep = _stream->ReadInt16();

    if (coldep == 0)
    {
        _lastLoad = index;
        return 0;
    }

    int wdd = _stream->ReadInt16();
    int htt = _stream->ReadInt16();
    // update the stored width/height
    _sprInfos[index].Width = wdd;
    _sprInfos[index].Height = htt;

    _spriteData[index].Image = BitmapHelper::CreateBitmap(wdd, htt, coldep * 8);
    if (_spriteData[index].Image == NULL)
    {
        _spriteData[index].Offset = 0;
        return 0;
    }

    Bitmap *image = _spriteData[index].Image;
    if (this->_compressed) 
    {
        _stream->ReadInt32(); // skip data size
        if (coldep == 1)
        {
            for (hh = 0; hh < htt; hh++)
                cunpackbitl(&image->GetScanLineForWriting(hh)[0], wdd, _stream);
        }
        else if (coldep == 2)
        {
            for (hh = 0; hh < htt; hh++)
                cunpackbitl16((unsigned short*)&image->GetScanLine(hh)[0], wdd, _stream);
        }
        else
        {
            for (hh = 0; hh < htt; hh++)
                cunpackbitl32((unsigned int*)&image->GetScanLine(hh)[0], wdd, _stream);
        }
    }
    else
    {
        if (coldep == 1)
        {
            for (hh = 0; hh < htt; hh++)
                _stream->ReadArray(&image->GetScanLineForWriting(hh)[0], coldep, wdd);
        }
        else if (coldep == 2)
        {
            for (hh = 0; hh < htt; hh++)
                _stream->ReadArrayOfInt16((int16_t*)&image->GetScanLineForWriting(hh)[0], wdd);
        }
        else
        {
            for (hh = 0; hh < htt; hh++)
                _stream->ReadArrayOfInt32((int32_t*)&image->GetScanLineForWriting(hh)[0], wdd);
        }
    }

    _lastLoad = index;

    // Stop it adding the sprite to the used list just because it's loaded
    // TODO: this messy hack is required, because initialize_sprite calls operator[]
    // which puts the sprite to the MRU list.
    _spriteData[index].Flags |= SPRCACHEFLAG_LOCKED;

    // TODO: this is ugly: asks the engine to convert the sprite using its own knowledge.
    // And engine assigns new bitmap using SpriteCache::Set().
    // Perhaps change to the callback function pointer?
    initialize_sprite(index);

    if (index != 0)  // leave sprite 0 locked
        _spriteData[index].Flags &= ~SPRCACHEFLAG_LOCKED;

    // we need to store this because the main program might
    // alter spritewidth/height if it resizes stuff
    size_t size = _sprInfos[index].Width * _sprInfos[index].Height * coldep;
    _spriteData[index].Size = size;
    _cacheSize += size;

#ifdef DEBUG_SPRITECACHE
    Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Debug, "Loaded %lld, size now %u KB", index, _cacheSize / 1024);
#endif

    return size;
}

const char *spriteFileSig = " Sprite File ";

void SpriteCache::compressSprite(Bitmap *sprite, Stream *out)
{
    int depth = sprite->GetColorDepth() / 8;

    if (depth == 1)
    {
        for (int yy = 0; yy < sprite->GetHeight(); yy++)
            cpackbitl(&sprite->GetScanLineForWriting(yy)[0], sprite->GetWidth(), out);
    }
    else if (depth == 2)
    {
        for (int yy = 0; yy < sprite->GetHeight(); yy++)
            cpackbitl16((unsigned short *)&sprite->GetScanLine(yy)[0], sprite->GetWidth(), out);
    }
    else
    {
        for (int yy = 0; yy < sprite->GetHeight(); yy++)
            cpackbitl32((unsigned int *)&sprite->GetScanLine(yy)[0], sprite->GetWidth(), out);
    }
}

int SpriteCache::saveToFile(const char *filnam, sprkey_t lastElement, bool compressOutput)
{
    Stream *output = Common::File::CreateFile(filnam);
    if (output == NULL)
        return -1;

    if (compressOutput)
    {
        // re-open the file so that it can be seeked
        delete output;
        output = File::OpenFile(filnam, Common::kFile_Open, Common::kFile_ReadWrite); // CHECKME why mode was "r+" here?
        if (output == NULL)
            return -1;
    }

    int spriteFileIDCheck = (int)time(NULL);

    // sprite file version
    output->WriteInt16(kSprfVersion_Current);

    output->WriteArray(spriteFileSig, strlen(spriteFileSig), 1);

    output->WriteInt8(compressOutput ? 1 : 0);
    output->WriteInt32(spriteFileIDCheck);

    sprkey_t lastslot = 0;
    if (lastElement < 0)
        lastElement = 0;
    if (_spriteData.size() < (size_t)lastElement)
        lastElement = (sprkey_t)_spriteData.size();

    for (sprkey_t i = 1; i < lastElement; i++)
    {
        // slot empty
        if ((_spriteData[i].Image != NULL) || ((_spriteData[i].Offset != 0) && (_spriteData[i].Offset != _sprite0InitialOffset)))
            lastslot = i;
    }

    output->WriteInt16(lastslot);

    // allocate buffers to store the indexing info
    sprkey_t numsprits = lastslot + 1;
    short *spritewidths = new short[numsprits];
    short *spriteheights = new short[numsprits];
    soff_t *spriteoffs = new soff_t[numsprits];

    const int memBufferSize = 100000;
    char *memBuffer = new char[memBufferSize];

    for (sprkey_t i = 0; i <= lastslot; i++)
    {
        spriteoffs[i] = output->GetPosition();

        // if compressing uncompressed sprites, load the sprite into memory
        if ((_spriteData[i].Image == NULL) && (this->_compressed != compressOutput))
            (*this)[i];

        if (_spriteData[i].Image != NULL)
        {
            // image in memory -- write it out
            pre_save_sprite(i);
            Bitmap *image = _spriteData[i].Image;
            int bpss = image->GetColorDepth() / 8;
            spritewidths[i] = image->GetWidth();
            spriteheights[i] = image->GetHeight();
            output->WriteInt16(bpss);
            output->WriteInt16(spritewidths[i]);
            output->WriteInt16(spriteheights[i]);

            if (compressOutput)
            {
                size_t lenloc = output->GetPosition();
                // write some space for the length data
                output->WriteInt32(0);

                compressSprite(image, output);

                size_t fileSizeSoFar = output->GetPosition();
                // write the length of the compressed data
                output->Seek(lenloc, kSeekBegin);
                output->WriteInt32((fileSizeSoFar - lenloc) - 4);
                output->Seek(0, kSeekEnd);
            }
            else
            {
                output->WriteArray(image->GetDataForWriting(), spritewidths[i] * bpss, spriteheights[i]);
            }
            continue;
        }

        if ((_spriteData[i].Offset == 0) || ((_spriteData[i].Offset == _sprite0InitialOffset) && (i > 0)))
        {
            // sprite doesn't exist
            output->WriteInt16(0);
            spritewidths[i] = 0;
            spriteheights[i] = 0;
            spriteoffs[i] = 0;
            continue;
        }

        // not in memory -- seek to it in the source file
        seekToSprite(i);
        _lastLoad = i;

        short colDepth = _stream->ReadInt16();
        output->WriteInt16(colDepth);

        if (colDepth == 0)
            continue;

        if (this->_compressed != compressOutput)
        {
            // shouldn't be able to get here
            delete [] memBuffer;
            delete output;
            return -2;
        }

        // and copy the data across
        int width = _stream->ReadInt16();
        int height = _stream->ReadInt16();

        spritewidths[i] = width;
        spriteheights[i] = height;

        output->WriteInt16(width);
        output->WriteInt16(height);

        int sizeToCopy;
        if (this->_compressed)
        {
            sizeToCopy = _stream->ReadInt32();
            output->WriteInt32(sizeToCopy);
        }
        else
        {
            sizeToCopy = width * height * (int)colDepth;
        }

        while (sizeToCopy > memBufferSize)
        {
            _stream->ReadArray(memBuffer, memBufferSize, 1);
            output->WriteArray(memBuffer, memBufferSize, 1);
            sizeToCopy -= memBufferSize;
        }

        _stream->ReadArray(memBuffer, sizeToCopy, 1);
        output->WriteArray(memBuffer, sizeToCopy, 1);
    }

    delete [] memBuffer;
    delete output;

    // write the sprite index file
    Stream *spindex_out = File::CreateFile(spindexfilename);
    // write "SPRINDEX" id
    spindex_out->WriteArray(&spindexid[0], strlen(spindexid), 1);
    // write version
    spindex_out->WriteInt32(kSpridxfVersion_Current);
    spindex_out->WriteInt32(spriteFileIDCheck);
    // write last sprite number and num sprites, to verify that
    // it matches the spr file
    spindex_out->WriteInt32(lastslot);
    spindex_out->WriteInt32(numsprits);
    spindex_out->WriteArrayOfInt16(&spritewidths[0], numsprits);
    spindex_out->WriteArrayOfInt16(&spriteheights[0], numsprits);
    spindex_out->WriteArrayOfInt64(&spriteoffs[0], numsprits);
    delete spindex_out;

    delete [] spritewidths;
    delete [] spriteheights;
    delete [] spriteoffs;
    return 0;
}

int SpriteCache::InitFile(const char *filnam)
{
    SpriteFileVersion vers;
    char buff[20];
    sprkey_t numspri = 0;
    int wdd, htt;
    soff_t spr_initial_offs = 0;
    int spriteFileID = 0;

    for (size_t i = 0; i < _spriteData.size(); ++i)
    {
        _spriteData[i] = SpriteData();
    }

    _stream = Common::AssetManager::OpenAsset((char *)filnam);
    if (_stream == NULL)
        return -1;

    spr_initial_offs = _stream->GetPosition();

    vers = (SpriteFileVersion)_stream->ReadInt16();
    // read the "Sprite File" signature
    _stream->ReadArray(&buff[0], 13, 1);

    if (vers < kSprfVersion_Uncompressed || vers > kSprfVersion_64bit)
    {
        delete _stream;
        _stream = NULL;
        return -1;
    }

    // unknown version
    buff[13] = 0;
    if (strcmp(buff, spriteFileSig))
    {
        delete _stream;
        _stream = NULL;
        return -1;
    }

    if (vers == kSprfVersion_Uncompressed)
    {
        this->_compressed = false;
    }
    else if (vers == kSprfVersion_Compressed)
    {
        this->_compressed = true;
    }
    else if (vers >= kSprfVersion_Last32bit)
    {
        this->_compressed = (_stream->ReadInt8() == 1);
        spriteFileID = _stream->ReadInt32();
    }

    if (vers < kSprfVersion_Compressed)
    {
        // skip the palette
        _stream->Seek(256 * 3); // sizeof(RGB) * 256
    }

    numspri = _stream->ReadInt16();
    if (vers < kSprfVersion_Uncompressed)
        numspri = 200;

    initFile_adjustBuffers(numspri);

    // if there is a sprite index file, use it
    if (loadSpriteIndexFile(spriteFileID, spr_initial_offs, numspri))
    {
        // Succeeded
        return 0;
    }

    // failed, delete the index file because it's invalid
    // TODO: refactor loading process and make it NOT delete file running the game!!
    unlink(spindexfilename);

    // no sprite index file, manually index it
    for (sprkey_t i = 0; i <= numspri; ++i)
    {
        _spriteData[i].Offset = _stream->GetPosition();
        _spriteData[i].Flags = 0;

        int coldep = _stream->ReadInt16();

        if (coldep == 0)
        {
            _spriteData[i].Offset = 0;
            _spriteData[i].Image = NULL;

            initFile_initNullSpriteParams(i);

            if (_stream->EOS())
                break;

            continue;
        }

        if (_stream->EOS())
            break;

        if ((size_t)i >= _spriteData.size())
            break;

        _spriteData[i].Image = NULL;

        wdd = _stream->ReadInt16();
        htt = _stream->ReadInt16();

        _sprInfos[i].Width = wdd;
        _sprInfos[i].Height = htt;
        get_new_size_for_sprite(i, wdd, htt, _sprInfos[i].Width, _sprInfos[i].Height);

        size_t spriteDataSize;
        if (vers == kSprfVersion_Compressed)
        {
            spriteDataSize = _stream->ReadInt32();
        }
        else if (vers >= kSprfVersion_Last32bit)
        {
            spriteDataSize = this->_compressed ? _stream->ReadInt32() :
            wdd * coldep * htt;
        }
        else
        {
            spriteDataSize = wdd * coldep * htt;
        }

        _stream->Seek(spriteDataSize);
    }

    _sprite0InitialOffset = _spriteData[0].Offset;
    return 0;
}

bool SpriteCache::loadSpriteIndexFile(int expectedFileID, soff_t spr_initial_offs, sprkey_t numspri)
{
    sprkey_t numspri_index = 0;
    Stream *fidx = Common::AssetManager::OpenAsset((char*)spindexfilename);
    if (fidx == NULL) 
    {
        return false;
    }

    char buffer[9];
    // check "SPRINDEX" id
    fidx->ReadArray(&buffer[0], strlen(spindexid), 1);
    buffer[8] = 0;
    if (strcmp(buffer, spindexid))
    {
        delete fidx;
        return false;
    }
    // check version
    SpriteIndexFileVersion fileVersion = (SpriteIndexFileVersion)fidx->ReadInt32();
    if (fileVersion < kSpridxfVersion_Initial || fileVersion > kSpridxfVersion_Current)
    {
        delete fidx;
        return false;
    }
    if (fileVersion >= kSpridxfVersion_Last32bit)
    {
        if (fidx->ReadInt32() != expectedFileID)
        {
            delete fidx;
            return false;
        }
    }

    numspri_index = fidx->ReadInt32();
    // end index+1 should be the same as num sprites
    if (fidx->ReadInt32() != numspri_index + 1)
    {
        delete fidx;
        return false;
    }

    if (numspri_index != numspri)
    {
        delete fidx;
        return false;
    }

    sprkey_t numsprits = numspri + 1;
    short *rspritewidths = new short[numsprits];
    short *rspriteheights = new short[numsprits];
    soff_t *spriteoffs = new soff_t[numsprits];

    fidx->ReadArrayOfInt16(&rspritewidths[0], numsprits);
    fidx->ReadArrayOfInt16(&rspriteheights[0], numsprits);
    if (fileVersion <= kSpridxfVersion_Last32bit)
    {
        for (sprkey_t i = 0; i < numsprits; ++i)
            spriteoffs[i] = fidx->ReadInt32();
    }
    else // large file support
    {
        fidx->ReadArrayOfInt64(spriteoffs, numsprits);
    }

    for (sprkey_t i = 0; i <= numspri; ++i)
    {
        _spriteData[i].Flags = 0;
        if (spriteoffs[i] != 0)
        {
            _spriteData[i].Offset = spriteoffs[i] + spr_initial_offs;
            get_new_size_for_sprite(i, rspritewidths[i], rspriteheights[i], _sprInfos[i].Width, _sprInfos[i].Height);
        }
        else if (i > 0)
        {
            initFile_initNullSpriteParams(i);
        }
    }

    _sprite0InitialOffset = _spriteData[0].Offset;
    delete [] rspritewidths;
    delete [] rspriteheights;
    delete [] spriteoffs;
    delete fidx;
    return true;
}

void SpriteCache::detachFile()
{
    delete _stream;
    _stream = NULL;
    _lastLoad = -2;
}

int SpriteCache::attachFile(const char *filename)
{
    _stream = Common::AssetManager::OpenAsset((char *)filename);
    if (_stream == NULL)
        return -1;
    return 0;
}
