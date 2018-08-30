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

#include <stdio.h> // sprintf
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
extern void get_new_size_for_sprite(int, int, int &, int &);
extern int spritewidth[], spriteheight[];

#define SPRITE_LOCKED -1
#define START_OF_LIST -1
#define END_OF_LIST   -1

const char *spindexid = "SPRINDEX";
const char *spindexfilename = "sprindex.dat";

// TODO: research old version differences
enum SpriteFileVersion
{
    kSprfVersion_Uncompressed   = 4,
    kSprfVersion_Compressed     = 5,
    kSprfVersion_Last32bit      = 6,
    kSprfVersion_64bit          = 10,
    kSprfVersion_Current        = kSprfVersion_64bit
};

enum SpriteIndexFileVersion
{
    kSpridxfVersion_Initial     = 1,
    kSpridxfVersion_Last32bit   = 2,
    kSpridxfVersion_64bit       = 10,
    kSpridxfVersion_Current     = kSpridxfVersion_64bit
};


SpriteCache::SpriteCache(int32_t maxElements)
{
  elements = maxElements;
  cache_stream = NULL;
  offsets = NULL;
  sprite0InitialOffset = 0;
  spritesAreCompressed = false;
  init();
}

void SpriteCache::changeMaxSize(int32_t maxElements) {
  elements = maxElements;
  if (offsets) {
    free(offsets);
    free(images);
    free(mrulist);
    free(mrubacklink);
    free(sizes);
    free(flags);
  }
  offsets = (soff_t *)calloc(elements, sizeof(soff_t));
  memset(offsets, 0, elements*sizeof(soff_t));
  images = (Bitmap **) calloc(elements, sizeof(Bitmap *));
  mrulist = (int *)calloc(elements, sizeof(int));
  mrubacklink = (int *)calloc(elements, sizeof(int));
  sizes = (soff_t *)calloc(elements, sizeof(soff_t));
  flags = (unsigned char *)calloc(elements, sizeof(unsigned char));
}

void SpriteCache::init()
{
  delete cache_stream;
  cache_stream = NULL;
  changeMaxSize(elements);
  cachesize = 0;
  lockedSize = 0;
  liststart = -1;
  listend = -1;
  lastLoad = -2;
  maxCacheSize = DEFAULTCACHESIZE;
}

void SpriteCache::reset()
{
  int ii;
  for (ii = 0; ii < elements; ii++) {
    if (images[ii] != NULL) {
      delete images[ii];
      images[ii] = NULL;
    }
  }

  free(offsets);
  free(images);
  free(mrulist);
  free(mrubacklink);
  free(sizes);
  free(flags);
  offsets = NULL;

  init();
}

void SpriteCache::set(int index, Bitmap *sprite)
{
  images[index] = sprite;
}

void SpriteCache::setNonDiscardable(int index, Bitmap *sprite)
{
  images[index] = sprite;
  offsets[index] = SPRITE_LOCKED;
}

void SpriteCache::removeSprite(int index, bool freeMemory)
{
  if ((images[index] != NULL) && (freeMemory))
    delete images[index];

  images[index] = NULL;
  offsets[index] = 0;
}

int SpriteCache::enlargeTo(int32_t newsize) {
  if (newsize <= elements)
    return 0;

  int elementsWas = elements;
  elements = newsize;
  offsets = (soff_t *)realloc(offsets, elements * sizeof(soff_t));
  images = (Bitmap **)realloc(images, elements * sizeof(Bitmap *));
  mrulist = (int *)realloc(mrulist, elements * sizeof(int));
  mrubacklink = (int *)realloc(mrubacklink, elements * sizeof(int));
  sizes = (soff_t *)realloc(sizes, elements * sizeof(soff_t));
  flags = (unsigned char*)realloc(flags, elements * sizeof(unsigned char));

  for (int i = elementsWas; i < elements; i++) {
    offsets[i] = 0;
    images[i] = 0;
    mrulist[i] = 0;
    mrubacklink[i] = 0;
    sizes[i] = 0;
    flags[i] = SPRCACHEFLAG_DOESNOTEXIST;
  }

  return elementsWas;
}

int SpriteCache::findFreeSlot()
{
  int i;
  for (i = 1; i < elements; i++) {
    // slot empty
    if ((images[i] == NULL) && ((offsets[i] == 0) || (offsets[i] == sprite0InitialOffset)))
      return i;
  }
  // no free slot found yet
  if (elements < MAX_SPRITES) {
    // enlarge the sprite bank to find a free slot
    // and return the first new free slot
    return enlargeTo(elements + 100);
  }
  return -1;
}

int SpriteCache::doesSpriteExist(int index) {
  if (images[index] != NULL)
    return 1;
  
  if (flags[index] & SPRCACHEFLAG_DOESNOTEXIST)
    return 0;

  if (offsets[index] > 0)
    return 1;

  return 0;
}

Bitmap *SpriteCache::operator [] (int index)
{
  // invalid sprite slot
  if ((index < 0) || (index >= elements))
    return NULL;

  // Dynamically added sprite, don't put it on the sprite list
  if ((images[index] != NULL) && 
      ((offsets[index] == 0) || ((flags[index] & SPRCACHEFLAG_DOESNOTEXIST) != 0)))
    return images[index];

  // if sprite exists in file but is not in mem, load it
  if ((images[index] == NULL) && (offsets[index] > 0))
    loadSprite(index);

  // Locked sprite, eg. mouse cursor, that shouldn't be discarded
  if (offsets[index] == SPRITE_LOCKED)
    return images[index];

  if (liststart < 0) {
    liststart = index;
    listend = index;
    mrulist[index] = END_OF_LIST;
    mrubacklink[index] = START_OF_LIST;
  } 
  else if (listend != index) {
    // this is the oldest element being bumped to newest, so update start link
    if (index == liststart) {
      liststart = mrulist[index];
      mrubacklink[liststart] = START_OF_LIST;
    }
    // already in list, link previous to next
    else if (mrulist[index] > 0) {
      mrulist[mrubacklink[index]] = mrulist[index];
      mrubacklink[mrulist[index]] = mrubacklink[index];
    }

    // set this as the newest element in the list
    mrulist[index] = END_OF_LIST;
    mrulist[listend] = index;
    mrubacklink[index] = listend;
    listend = index;
  }

  return images[index];
}

// Remove the oldest cache element
void SpriteCache::removeOldest()
{

  if (liststart < 0)
    return;

  int sprnum = liststart;

  if ((images[sprnum] != NULL) && (offsets[sprnum] != SPRITE_LOCKED)) {
    // Free the memory
    if (flags[sprnum] & SPRCACHEFLAG_DOESNOTEXIST)
    {
      char msgg[150];
      sprintf(msgg, "SpriteCache::removeOldest: Attempted to remove sprite %d that does not exist", sprnum);
      quit(msgg);
    }
    cachesize -= sizes[sprnum];

    delete images[sprnum];
    images[sprnum] = NULL;
  }

  if (liststart == listend)
  {
    // there was one huge sprite, removing it has now emptied the cache completely
    if (cachesize > 0)
    {
      Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "SPRITE CACHE ERROR: Sprite cache should be empty, but still has %d bytes", cachesize);
    }
    mrulist[liststart] = 0;
    liststart = -1;
    listend = -1;
  }
  else
  {
    int oldstart = liststart;
    liststart = mrulist[liststart];
    mrulist[oldstart] = 0;
    mrubacklink[liststart] = START_OF_LIST;
    if (oldstart == liststart) {
      // Somehow, we have got a recursive link to itself, so we
      // the game will freeze (since it is not actually freeing any
      // memory)
      // There must be a bug somewhere causing this, but for now
      // let's just reset the cache
      Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "RUNTIME CACHE ERROR: CACHE INCONSISTENT: RESETTING\n\tAt size %d (of %d), start %d end %d  fwdlink=%d",
                    cachesize, maxCacheSize, oldstart, listend, liststart);
      removeAll();
    }
  }

#ifdef DEBUG_SPRITECACHE
  Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Debug, "Removed %d, size now %d KB", sprnum, cachesize / 1024);
#endif
}

void SpriteCache::removeAll()
{
  int ii;

  liststart = -1;
  listend = -1;
  for (ii = 0; ii < elements; ii++) {
    if ((offsets[ii] != SPRITE_LOCKED) && (images[ii] != NULL) &&
        ((flags[ii] & SPRCACHEFLAG_DOESNOTEXIST) == 0)) 
    {
      delete images[ii];
      images[ii] = NULL;
    }
    mrulist[ii] = 0;
    mrubacklink[ii] = 0;
  }
  cachesize = lockedSize;
}

void SpriteCache::precache(int index)
{
  if ((index < 0) || (index >= elements))
    return;

  soff_t sprSize = 0;

  if (images[index] == NULL) {
    sprSize = loadSprite(index);
  }
  else if (offsets[index] != SPRITE_LOCKED) {
    sprSize = sizes[index];
  }

  // make sure locked sprites can't fill the cache
  maxCacheSize += sprSize;
  lockedSize += sprSize;

  offsets[index] = SPRITE_LOCKED;

#ifdef DEBUG_SPRITECACHE
  Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Debug, "Precached %d", index);
#endif
}

void SpriteCache::seekToSprite(int index) {
  if (index - 1 != lastLoad)
      cache_stream->Seek(offsets[index], kSeekBegin);
}

soff_t SpriteCache::loadSprite(int index)
{
  int hh = 0;

  while (cachesize > maxCacheSize) {
    removeOldest();
    hh++;
    if (hh > 1000) {
      Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Error, "RUNTIME CACHE ERROR: STUCK IN FREE_UP_MEM; RESETTING CACHE");
      removeAll();
    }

  }

  if ((index < 0) || (index >= elements))
    quit("sprite cache array index out of bounds");

  // If we didn't just load the previous sprite, seek to it
  seekToSprite(index);

  int coldep = cache_stream->ReadInt16();

  if (coldep == 0) {
    lastLoad = index;
    return 0;
  }

  int wdd = cache_stream->ReadInt16();
  int htt = cache_stream->ReadInt16();
  // update the stored width/height
  spritewidth[index] = wdd;
  spriteheight[index] = htt;

  images[index] = BitmapHelper::CreateBitmap(wdd, htt, coldep * 8);
  if (images[index] == NULL) {
    offsets[index] = 0;
    return 0;
  }

  if (this->spritesAreCompressed) 
  {
    cache_stream->ReadInt32(); // skip data size
    if (coldep == 1) {
      for (hh = 0; hh < htt; hh++)
        cunpackbitl(&images[index]->GetScanLineForWriting(hh)[0], wdd, cache_stream);
    }
    else if (coldep == 2) {
      for (hh = 0; hh < htt; hh++)
        cunpackbitl16((unsigned short*)&images[index]->GetScanLine(hh)[0], wdd, cache_stream);
    }
    else {
      for (hh = 0; hh < htt; hh++)
        cunpackbitl32((unsigned int*)&images[index]->GetScanLine(hh)[0], wdd, cache_stream);
    }
  }
  else {
    if (coldep == 1)
    {
      for (hh = 0; hh < htt; hh++)
        cache_stream->ReadArray(&images[index]->GetScanLineForWriting(hh)[0], coldep, wdd);
    }
    else if (coldep == 2)
    {
      for (hh = 0; hh < htt; hh++)
        cache_stream->ReadArrayOfInt16((int16_t*)&images[index]->GetScanLineForWriting(hh)[0], wdd);
    }
    else
    {
      for (hh = 0; hh < htt; hh++)
        cache_stream->ReadArrayOfInt32((int32_t*)&images[index]->GetScanLineForWriting(hh)[0], wdd);
    }
  }

  lastLoad = index;

  // Stop it adding the sprite to the used list just because it's loaded
  soff_t offs = offsets[index];
  offsets[index] = SPRITE_LOCKED;

  initialize_sprite(index);

  if (index != 0)  // leave sprite 0 locked
    offsets[index] = offs;

  // we need to store this because the main program might
  // alter spritewidth/height if it resizes stuff
  sizes[index] = spritewidth[index] * spriteheight[index] * coldep;
  cachesize += sizes[index];

#ifdef DEBUG_SPRITECACHE
  Debug::Printf(kDbgGroup_SprCache, kDbgMsg_Debug, "Loaded %d, size now %lld KB", index, cachesize / 1024);
#endif

  return sizes[index];
}

const char *spriteFileSig = " Sprite File ";

void SpriteCache::compressSprite(Bitmap *sprite, Stream *out) {

  int depth = sprite->GetColorDepth() / 8;

  if (depth == 1) {
    for (int yy = 0; yy < sprite->GetHeight(); yy++)
      cpackbitl(&sprite->GetScanLineForWriting(yy)[0], sprite->GetWidth(), out);
  }
  else if (depth == 2) {
    for (int yy = 0; yy < sprite->GetHeight(); yy++)
      cpackbitl16((unsigned short *)&sprite->GetScanLine(yy)[0], sprite->GetWidth(), out);
  }
  else {
    for (int yy = 0; yy < sprite->GetHeight(); yy++)
      cpackbitl32((unsigned int *)&sprite->GetScanLine(yy)[0], sprite->GetWidth(), out);
  }

}

int SpriteCache::saveToFile(const char *filnam, int lastElement, bool compressOutput)
{
  Stream *output = Common::File::CreateFile(filnam);
  if (output == NULL)
    return -1;

  if (compressOutput) {
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

  int i, lastslot = 0;
  if (elements < lastElement)
    lastElement = elements;

  for (i = 1; i < lastElement; i++) {
    // slot empty
    if ((images[i] != NULL) || ((offsets[i] != 0) && (offsets[i] != sprite0InitialOffset)))
      lastslot = i;
  }

  output->WriteInt16(lastslot);

  // allocate buffers to store the indexing info
  int numsprits = lastslot + 1;
  short *spritewidths = (short*)malloc(numsprits * sizeof(short));
  short *spriteheights = (short*)malloc(numsprits * sizeof(short));
  soff_t *spriteoffs = (soff_t*)malloc(numsprits * sizeof(soff_t));

  const int memBufferSize = 100000;
  char *memBuffer = (char*)malloc(memBufferSize);

  for (i = 0; i <= lastslot; i++) {

    spriteoffs[i] = output->GetPosition();

    // if compressing uncompressed sprites, load the sprite into memory
    if ((images[i] == NULL) && (this->spritesAreCompressed != compressOutput))
      (*this)[i];

    if (images[i] != NULL) {
      // image in memory -- write it out
      pre_save_sprite(i);
      int bpss = images[i]->GetColorDepth() / 8;
      spritewidths[i] = images[i]->GetWidth();
      spriteheights[i] = images[i]->GetHeight();
      output->WriteInt16(bpss);
      output->WriteInt16(spritewidths[i]);
      output->WriteInt16(spriteheights[i]);

      if (compressOutput) {
        size_t lenloc = output->GetPosition();
        // write some space for the length data
        output->WriteInt32(0);

        compressSprite(images[i], output);

        size_t fileSizeSoFar = output->GetPosition();
        // write the length of the compressed data
        output->Seek(lenloc, kSeekBegin);
        output->WriteInt32((fileSizeSoFar - lenloc) - 4);
        output->Seek(0, kSeekEnd);
      }
      else
        output->WriteArray(images[i]->GetDataForWriting(), spritewidths[i] * bpss, spriteheights[i]);

      continue;
    }

    if ((offsets[i] == 0) || ((offsets[i] == sprite0InitialOffset) && (i > 0))) {
      // sprite doesn't exist
      output->WriteInt16(0);
      spritewidths[i] = 0;
      spriteheights[i] = 0;
      spriteoffs[i] = 0;
      continue;
    }

    // not in memory -- seek to it in the source file
    seekToSprite(i);
    lastLoad = i;

    short colDepth = cache_stream->ReadInt16();
    output->WriteInt16(colDepth);

    if (colDepth == 0) {
      continue;
    }

    if (this->spritesAreCompressed != compressOutput) {
      // shouldn't be able to get here
      free(memBuffer);
      delete output;
      return -2;
    }

    // and copy the data across
    int width = cache_stream->ReadInt16();
    int height = cache_stream->ReadInt16();

    spritewidths[i] = width;
    spriteheights[i] = height;

    output->WriteInt16(width);
    output->WriteInt16(height);

    int sizeToCopy;
    if (this->spritesAreCompressed) {
      sizeToCopy = cache_stream->ReadInt32();
      output->WriteInt32(sizeToCopy);
    }
    else {
      sizeToCopy = width * height * (int)colDepth;
    }

    while (sizeToCopy > memBufferSize) {
      cache_stream->ReadArray(memBuffer, memBufferSize, 1);
      output->WriteArray(memBuffer, memBufferSize, 1);
      sizeToCopy -= memBufferSize;
    }

    cache_stream->ReadArray(memBuffer, sizeToCopy, 1);
    output->WriteArray(memBuffer, sizeToCopy, 1);
  }

  free(memBuffer);

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

  free(spritewidths);
  free(spriteheights);
  free(spriteoffs);

  return 0;
}

int SpriteCache::initFile(const char *filnam)
{
  short vers;
  char buff[20];
  short numspri = 0;
  int vv, wdd, htt;
  int32_t spr_initial_offs = 0;
  int spriteFileID = 0;

  for (vv = 0; vv < elements; vv++) {
    images[vv] = NULL;
    offsets[vv] = 0;
  }

  cache_stream = Common::AssetManager::OpenAsset((char *)filnam);
  if (cache_stream == NULL)
    return -1;

  spr_initial_offs = cache_stream->GetPosition();

  vers = cache_stream->ReadInt16();
  // read the "Sprite File" signature
  cache_stream->ReadArray(&buff[0], 13, 1);

  if ((vers < kSprfVersion_Uncompressed) || (vers > kSprfVersion_64bit)) {
    delete cache_stream;
    cache_stream = NULL;
    return -1;
  }

  // unknown version
  buff[13] = 0;
  if (strcmp(buff, spriteFileSig)) {
    delete cache_stream;
    cache_stream = NULL;
    return -1;
  }

  if (vers == kSprfVersion_Uncompressed)
    this->spritesAreCompressed = false;
  else if (vers == kSprfVersion_Compressed)
    this->spritesAreCompressed = true;
  else if (vers >= kSprfVersion_Last32bit)
  {
    this->spritesAreCompressed = (cache_stream->ReadInt8() == 1);
    spriteFileID = cache_stream->ReadInt32();
  }

  if (vers < kSprfVersion_Compressed) {
    // skip the palette
      cache_stream->Seek(256 * 3);
  }

  numspri = cache_stream->ReadInt16();

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
  unlink(spindexfilename);

  // no sprite index file, manually index it

  for (vv = 0; vv <= numspri; vv++) {

    offsets[vv] = cache_stream->GetPosition();
    flags[vv] = 0;

    int coldep = cache_stream->ReadInt16();

    if (coldep == 0) {
      offsets[vv] = 0;
      images[vv] = NULL;

      initFile_initNullSpriteParams(vv);

      if (cache_stream->EOS())
        break;

      continue;
    }

    if (cache_stream->EOS())
      break;

    if (vv >= elements)
      break;

    images[vv] = NULL;

    wdd = cache_stream->ReadInt16();
    htt = cache_stream->ReadInt16();

    spritewidth[vv] = wdd;
    spriteheight[vv] = htt;
    get_new_size_for_sprite(wdd, htt, spritewidth[vv], spriteheight[vv]);

    int32_t spriteDataSize;
    if (vers == kSprfVersion_Compressed) {
      spriteDataSize = cache_stream->ReadInt32();
    }
    else if (vers >= kSprfVersion_Last32bit)
    {
      spriteDataSize = this->spritesAreCompressed ? cache_stream->ReadInt32() :
        wdd * coldep * htt;
    }
    else {
      spriteDataSize = wdd * coldep * htt;
    }

    cache_stream->Seek(spriteDataSize);
  }

  sprite0InitialOffset = offsets[0];
  return 0;
}

bool SpriteCache::loadSpriteIndexFile(int expectedFileID, soff_t spr_initial_offs, short numspri)
{
  short numspri_index = 0;
  int vv;
  Stream *fidx = Common::AssetManager::OpenAsset((char*)spindexfilename);
  if (fidx == NULL) 
  {
    return false;
  }

  char buffer[9];
  // check "SPRINDEX" id
  fidx->ReadArray(&buffer[0], strlen(spindexid), 1);
  buffer[8] = 0;
  if (strcmp(buffer, spindexid)) {
    delete fidx;
    return false;
  }
  // check version
  SpriteIndexFileVersion fileVersion = (SpriteIndexFileVersion)fidx->ReadInt32();
  if ((fileVersion < kSpridxfVersion_Initial) || (fileVersion > kSpridxfVersion_Current)) {
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
  if (fidx->ReadInt32() != numspri_index + 1) {
    delete fidx;
    return false;
  }

  if (numspri_index != numspri) {
    delete fidx;
    return false;
  }

  short numsprits = numspri + 1;
  short *rspritewidths = (short*)malloc(numsprits * sizeof(short));
  short *rspriteheights = (short*)malloc(numsprits * sizeof(short));

  fidx->ReadArrayOfInt16(&rspritewidths[0], numsprits);
  fidx->ReadArrayOfInt16(&rspriteheights[0], numsprits);
  if (fileVersion <= kSpridxfVersion_Last32bit)
  {
      for (int i = 0; i < numsprits; ++i)
          offsets[i] = fidx->ReadInt32();
  }
  else // large file support
  {
      fidx->ReadArrayOfInt64(&offsets[0], numsprits);
  }

  for (vv = 0; vv <= numspri; vv++) {
    flags[vv] = 0;
    if (offsets[vv] != 0) {
      offsets[vv] += spr_initial_offs;
      get_new_size_for_sprite(rspritewidths[vv], rspriteheights[vv], spritewidth[vv], spriteheight[vv]);
    }
    else if (vv > 0) {
      initFile_initNullSpriteParams(vv);
    }
  }

  sprite0InitialOffset = offsets[0];
  free(rspritewidths);
  free(rspriteheights);

  delete fidx;
  return true;
}

void SpriteCache::detachFile() {
  delete cache_stream;
  cache_stream = NULL;
  lastLoad = -2;
}

int SpriteCache::attachFile(const char *filename) {
  cache_stream = Common::AssetManager::OpenAsset((char *)filename);
  if (cache_stream == NULL)
    return -1;
  return 0;
}

