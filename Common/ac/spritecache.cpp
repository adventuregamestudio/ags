/*
** SPRCACHE - sprite caching system
** Copyright (C) 2002-2005, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifdef _MANAGED
// ensure this doesn't get compiled to .NET IL
#pragma unmanaged
#pragma warning (disable: 4996 4312)  // disable deprecation warnings
#endif

#include "util/wgt2allg.h"
#include "ac/common_defines.h"
#include "ac/spritecache.h"
#include "util/compress.h"
#include "util/file.h"

extern "C" {
  extern FILE *clibfopen(char *, char *);
}

//#define DEBUG_SPRITECACHE
// [IKM] We have to forward-declare these because their implementations are in the Engine
extern void write_log(char *);
extern void quit(char *);
extern void initialize_sprite(int);
extern void pre_save_sprite(int);
extern void get_new_size_for_sprite(int, int, int, int &, int &);
extern int spritewidth[], spriteheight[];

#define SPRITE_LOCKED -1
#define START_OF_LIST -1
#define END_OF_LIST   -1
// PSP: Use smaller sprite cache due to limited total memory.
#define DEFAULTCACHESIZE 5000000 //20240000        // max size, in bytes (20 MB)


SpriteCache::SpriteCache(long maxElements)
{
  elements = maxElements;
  ff = NULL;
  offsets = NULL;
  sprite0InitialOffset = 0;
  spritesAreCompressed = false;
  init();
}

void SpriteCache::changeMaxSize(long maxElements) {
  elements = maxElements;
  if (offsets) {
    free(offsets);
    free(images);
    free(mrulist);
    free(mrubacklink);
    free(sizes);
    free(flags);
  }
  offsets = (long *)calloc(elements, sizeof(long));
  images = (block *) calloc(elements, sizeof(block));
  mrulist = (int *)calloc(elements, sizeof(int));
  mrubacklink = (int *)calloc(elements, sizeof(int));
  sizes = (int *)calloc(elements, sizeof(int));
  flags = (unsigned char *)calloc(elements, sizeof(unsigned char));
}

void SpriteCache::init()
{
  if (ff != NULL)
    fclose(ff);

  ff = NULL;
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
      wfreeblock(images[ii]);
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

void SpriteCache::set(int index, block sprite)
{
  images[index] = sprite;
}

void SpriteCache::setNonDiscardable(int index, block sprite)
{
  images[index] = sprite;
  offsets[index] = SPRITE_LOCKED;
}

void SpriteCache::removeSprite(int index, bool freeMemory)
{
  if ((images[index] != NULL) && (freeMemory))
    wfreeblock(images[index]);

  images[index] = NULL;
  offsets[index] = 0;
}

int SpriteCache::enlargeTo(long newsize) {
  if (newsize <= elements)
    return 0;

  int elementsWas = elements;
  elements = newsize;
  offsets = (long *)realloc(offsets, elements * sizeof(long));
  images = (block *)realloc(images, elements * sizeof(block));
  mrulist = (int *)realloc(mrulist, elements * sizeof(int));
  mrubacklink = (int *)realloc(mrubacklink, elements * sizeof(int));
  sizes = (int *)realloc(sizes, elements * sizeof(int));
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

block SpriteCache::operator [] (int index)
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

/*    char bbb[50];   // Print out the MRU list
    sprintf(bbb, "Used %d, list is:", index);
    write_log(bbb);

    for (int i = liststart; 1; i = mrulist[i]) {
      
      sprintf(bbb, "%d", i);
      write_log(bbb);
      if (i == listend) break;
    }*/
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

    wfreeblock(images[sprnum]);
    images[sprnum] = NULL;
  }

  if (liststart == listend)
  {
    // there was one huge sprite, removing it has now emptied the cache completely
    if (cachesize > 0)
    {
      char msgg[150];
      sprintf(msgg, "!!!! SPRITE CACHE ERROR: Sprite cache should be empty, but still has %d bytes", cachesize);
      write_log(msgg);
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
      write_log("!!!! RUNTIME CACHE ERROR: CACHE INCONSISTENT: RESETTING");
      char msgg[150];
      sprintf(msgg, "!!!! At size %ld (of %ld), start %d end %d  fwdlink=%d",
              cachesize, maxCacheSize, oldstart, listend, liststart);
      write_log(msgg);

      removeAll();
    }
  }
#ifdef DEBUG_SPRITECACHE
  char msgg[100];
  sprintf(msgg, "Removed %d, size now %d KB", sprnum, cachesize / 1024);
  write_log(msgg);
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
      wfreeblock(images[ii]);
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

  int sprSize = 0;

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
  char msgg[100];
  sprintf(msgg, "Precached %d", index);
  write_log(msgg);
#endif
}

void SpriteCache::seekToSprite(int index) {
  if (index - 1 != lastLoad)
    fseek(ff, offsets[index], SEEK_SET);
}

int SpriteCache::loadSprite(int index)
{

  int hh = 0;

  while (cachesize > maxCacheSize) {
    removeOldest();
    hh++;
    if (hh > 1000) {
      write_log("!!!! RUNTIME CACHE ERROR: STUCK IN FREE_UP_MEM; RESETTING CACHE");
      removeAll();
    }

  }

  if ((index < 0) || (index >= elements))
    quit("sprite cache array index out of bounds");

  // If we didn't just load the previous sprite, seek to it
  seekToSprite(index);

  int coldep = getshort(ff);

  if (coldep == 0) {
    lastLoad = index;
    return 0;
  }

  int wdd = getshort(ff);
  int htt = getshort(ff);
  // update the stored width/height
  spritewidth[index] = wdd;
  spriteheight[index] = htt;

  images[index] = create_bitmap_ex(coldep * 8, wdd, htt);
  if (images[index] == NULL) {
    offsets[index] = 0;
    return 0;
  }

  if (this->spritesAreCompressed) 
  {
    getw(ff); // skip data size
    if (coldep == 1) {
      for (hh = 0; hh < htt; hh++)
        cunpackbitl(&images[index]->line[hh][0], wdd, ff);
    }
    else if (coldep == 2) {
      for (hh = 0; hh < htt; hh++)
        cunpackbitl16((unsigned short*)&images[index]->line[hh][0], wdd, ff);
    }
    else {
      for (hh = 0; hh < htt; hh++)
        cunpackbitl32((unsigned long*)&images[index]->line[hh][0], wdd, ff);
    }
  }
  else {
    for (hh = 0; hh < htt; hh++)
      // MACPORT FIX: size and nmemb split
      fread(&images[index]->line[hh][0], coldep, wdd, ff);
  }

  lastLoad = index;

  // Stop it adding the sprite to the used list just because it's loaded
  long offs = offsets[index];
  offsets[index] = SPRITE_LOCKED;

  initialize_sprite(index);

  if (index != 0)  // leave sprite 0 locked
    offsets[index] = offs;

  // we need to store this because the main program might
  // alter spritewidth/height if it resizes stuff
  sizes[index] = spritewidth[index] * spriteheight[index] * coldep;
  cachesize += sizes[index];

#ifdef DEBUG_SPRITECACHE
  char msgg[100];
  sprintf(msgg, "Loaded %d, size now %d KB", index, cachesize / 1024);
  write_log(msgg);
#endif

  return sizes[index];
}

const char *spriteFileSig = " Sprite File ";

void SpriteCache::compressSprite(block sprite, FILE *ooo) {

  int depth = bitmap_color_depth(sprite) / 8;

  if (depth == 1) {
    for (int yy = 0; yy < sprite->h; yy++)
      cpackbitl(&sprite->line[yy][0], sprite->w, ooo);
  }
  else if (depth == 2) {
    for (int yy = 0; yy < sprite->h; yy++)
      cpackbitl16((unsigned short *)&sprite->line[yy][0], sprite->w, ooo);
  }
  else {
    for (int yy = 0; yy < sprite->h; yy++)
      cpackbitl32((unsigned long *)&sprite->line[yy][0], sprite->w, ooo);
  }

}

int SpriteCache::saveToFile(const char *filnam, int lastElement, bool compressOutput)
{
  FILE *output = fopen(filnam, "wb");
  if (output == NULL)
    return -1;

  if (compressOutput) {
    // re-open the file so that it can be seeked
    fclose(output);
    output = fopen(filnam, "r+b");
    if (output == NULL)
      return -1;
  }

  int spriteFileIDCheck = (int)time(NULL);

  // version 6
  putshort(6, output);

  fwrite(spriteFileSig, strlen(spriteFileSig), 1, output);

  fputc(compressOutput ? 1 : 0, output);
  putw(spriteFileIDCheck, output);

  int i, lastslot = 0;
  if (elements < lastElement)
    lastElement = elements;

  for (i = 1; i < lastElement; i++) {
    // slot empty
    if ((images[i] != NULL) || ((offsets[i] != 0) && (offsets[i] != sprite0InitialOffset)))
      lastslot = i;
  }

  putshort(lastslot, output);

  // allocate buffers to store the indexing info
  int numsprits = lastslot + 1;
  short *spritewidths = (short*)malloc(numsprits * sizeof(short));
  short *spriteheights = (short*)malloc(numsprits * sizeof(short));
  long *spriteoffs = (long*)malloc(numsprits * sizeof(long));

  const int memBufferSize = 100000;
  char *memBuffer = (char*)malloc(memBufferSize);

  for (i = 0; i <= lastslot; i++) {

    spriteoffs[i] = ftell(output);

    // if compressing uncompressed sprites, load the sprite into memory
    if ((images[i] == NULL) && (this->spritesAreCompressed != compressOutput))
      (*this)[i];

    if (images[i] != NULL) {
      // image in memory -- write it out
      pre_save_sprite(i);
      int bpss = bitmap_color_depth(images[i]) / 8;
      spritewidths[i] = images[i]->w;
      spriteheights[i] = images[i]->h;
      putshort(bpss, output);
      putshort(spritewidths[i], output);
      putshort(spriteheights[i], output);

      if (compressOutput) {
        long lenloc = ftell(output);
        // write some space for the length data
        putw(0, output);

        compressSprite(images[i], output);

        long fileSizeSoFar = ftell(output);
        // write the length of the compressed data
        fseek(output, lenloc, SEEK_SET);
        putw((fileSizeSoFar - lenloc) - 4, output);
        fseek(output, 0, SEEK_END);
      }
      else
        fwrite(&images[i]->line[0][0], spritewidths[i] * bpss, spriteheights[i], output);

      continue;
    }

    if ((offsets[i] == 0) || ((offsets[i] == sprite0InitialOffset) && (i > 0))) {
      // sprite doesn't exist
      putshort(0, output);
      spritewidths[i] = 0;
      spriteheights[i] = 0;
      spriteoffs[i] = 0;
      continue;
    }

    // not in memory -- seek to it in the source file
    seekToSprite(i);
    lastLoad = i;

    short colDepth = getshort(ff);
    putshort(colDepth, output);

    if (colDepth == 0) {
      continue;
    }

    if (this->spritesAreCompressed != compressOutput) {
      // shouldn't be able to get here
      free(memBuffer);
      fclose(output);
      return -2;
    }

    // and copy the data across
    int width = getshort(ff);
    int height = getshort(ff);

    spritewidths[i] = width;
    spriteheights[i] = height;

    putshort(width, output);
    putshort(height, output);

    int sizeToCopy;
    if (this->spritesAreCompressed) {
      sizeToCopy = getw(ff);
      putw(sizeToCopy, output);
    }
    else {
      sizeToCopy = width * height * (int)colDepth;
    }

    while (sizeToCopy > memBufferSize) {
      fread(memBuffer, memBufferSize, 1, ff);
      fwrite(memBuffer, memBufferSize, 1, output);
      sizeToCopy -= memBufferSize;
    }

    fread(memBuffer, sizeToCopy, 1, ff);
    fwrite(memBuffer, sizeToCopy, 1, output);
  }

  free(memBuffer);

  fclose(output);

  // write the sprite index file
  FILE *ooo = fopen(spindexfilename, "wb");
  // write "SPRINDEX" id
  fwrite(&spindexid[0], strlen(spindexid), 1, ooo);
  // write version (1)
  putw(2, ooo);
  putw(spriteFileIDCheck, ooo);
  // write last sprite number and num sprites, to verify that
  // it matches the spr file
  putw(lastslot, ooo);
  putw(numsprits, ooo);
  fwrite(&spritewidths[0], sizeof(short), numsprits, ooo);
  fwrite(&spriteheights[0], sizeof(short), numsprits, ooo);
  fwrite(&spriteoffs[0], sizeof(long), numsprits, ooo);
  fclose(ooo);

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
  long spr_initial_offs = 0;
  int spriteFileID = 0;

  for (vv = 0; vv < elements; vv++) {
    images[vv] = NULL;
    offsets[vv] = 0;
  }

  ff = clibfopen((char *)filnam, "rb");
  if (ff == NULL)
    return -1;

  spr_initial_offs = ftell(ff);

  fread(&vers, 2, 1, ff);
  // read the "Sprite File" signature
  fread(&buff[0], 13, 1, ff);

  if ((vers < 4) || (vers > 6)) {
    fclose(ff);
    ff = NULL;
    return -1;
  }

  // unknown version
  buff[13] = 0;
  if (strcmp(buff, spriteFileSig)) {
    fclose(ff);
    ff = NULL;
    return -1;
  }

  if (vers == 4)
    this->spritesAreCompressed = false;
  else if (vers == 5)
    this->spritesAreCompressed = true;
  else if (vers >= 6)
  {
    this->spritesAreCompressed = (fgetc(ff) == 1);
    spriteFileID = getw(ff);
  }

  if (vers < 5) {
    // skip the palette
    fseek(ff, 256 * 3, SEEK_CUR);
  }

  fread(&numspri, 2, 1, ff);

  if (vers < 4)
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

    offsets[vv] = ftell(ff);
    flags[vv] = 0;

    int coldep = getshort(ff);

    if (coldep == 0) {
      offsets[vv] = 0;
      images[vv] = NULL;

      initFile_initNullSpriteParams(vv);

      if (feof(ff))
        break;

      continue;
    }

    if (feof(ff))
      break;

    if (vv >= elements)
      break;

    images[vv] = NULL;

    wdd = getshort(ff);
    htt = getshort(ff);

    spritewidth[vv] = wdd;
    spriteheight[vv] = htt;
    get_new_size_for_sprite(vv, wdd, htt, spritewidth[vv], spriteheight[vv]);

    long spriteDataSize;
    if (vers == 5) {
      spriteDataSize = getw(ff);
    }
    else {
      spriteDataSize = wdd * coldep * htt;
    }

    fseek(ff, spriteDataSize, SEEK_CUR);
  }

  sprite0InitialOffset = offsets[0];
  return 0;
}

bool SpriteCache::loadSpriteIndexFile(int expectedFileID, long spr_initial_offs, short numspri)
{
  short numspri_index = 0;
  int vv;
  FILE *fidx = clibfopen((char*)spindexfilename, "rb");
  if (fidx == NULL) 
  {
    return false;
  }

  char buffer[9];
  // check "SPRINDEX" id
  fread(&buffer[0], strlen(spindexid), 1, fidx);
  buffer[8] = 0;
  if (strcmp(buffer, spindexid)) {
    fclose(fidx);
    return false;
  }
  // check version
  int fileVersion = getw(fidx);
  if ((fileVersion < 1) || (fileVersion > 2)) {
    fclose(fidx);
    return false;
  }
  if (fileVersion >= 2)
  {
    if (getw(fidx) != expectedFileID)
    {
      fclose(fidx);
      return false;
    }
  }
  numspri_index = getw(fidx);

  // end index+1 should be the same as num sprites
  if (getw(fidx) != numspri_index + 1) {
    fclose(fidx);
    return false;
  }

  if (numspri_index != numspri) {
    fclose(fidx);
    return false;
  }

  short numsprits = numspri + 1;
  short *rspritewidths = (short*)malloc(numsprits * sizeof(short));
  short *rspriteheights = (short*)malloc(numsprits * sizeof(short));

  fread(&rspritewidths[0], sizeof(short), numsprits, fidx);
  fread(&rspriteheights[0], sizeof(short), numsprits, fidx);
  fread(&offsets[0], sizeof(long), numsprits, fidx);

  for (vv = 0; vv <= numspri; vv++) {
    flags[vv] = 0;
    if (offsets[vv] != 0) {
      offsets[vv] += spr_initial_offs;
      get_new_size_for_sprite(vv, rspritewidths[vv], rspriteheights[vv], spritewidth[vv], spriteheight[vv]);
    }
    else if (vv > 0) {
      initFile_initNullSpriteParams(vv);
    }
  }

  sprite0InitialOffset = offsets[0];
  free(rspritewidths);
  free(rspriteheights);

  fclose(fidx);
  return true;
}

void SpriteCache::detachFile() {
  if (ff != NULL)
    fclose(ff);
  ff = NULL;
  lastLoad = -2;
}

int SpriteCache::attachFile(const char *filename) {
  ff = clibfopen((char *)filename, "rb");
  if (ff == NULL)
    return -1;
  return 0;
}

