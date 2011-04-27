/*
** SPRCACHE.H - sprite caching system
** Copyright (C) 2002, Chris Jones
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE;
** the contents of this file may not be disclosed to third parties,
** copied or duplicated in any form, in whole or in part, without
** prior express permission from Chris Jones.
**
*/

#ifndef __SPRCACHE_H
#define __SPRCACHE_H

// We can't rely on offsets[slot]==0 because when the engine is running
// this is changed to reference the Bluecup sprite. Therefore we need
// a definite way of knowing whether the sprite existed in the sprite file.
#define SPRCACHEFLAG_DOESNOTEXIST 1

class SpriteCache
{
public:
  SpriteCache(long maxElements);

  int  initFile(const char *);
  int  loadSprite(int);
  void seekToSprite(int index);
  void precache(int);           // preloads and locks in memory
  void set(int, block);
  void setNonDiscardable(int, block);
  void removeSprite(int, bool);
  void removeOldest();
  void reset();                 // wipes all data 
  void init();
  void changeMaxSize(long);
  int  enlargeTo(long);
  void removeAll();             // removes all items from the cache
  int  findFreeSlot();
  int  saveToFile(const char *, int lastElement, bool compressOutput);
  int  doesSpriteExist(int index);
  void detachFile();
  int  attachFile(const char *);

  block operator[] (int index);

  long *offsets;
  long sprite0InitialOffset;
  long elements;                // size of offsets/images arrays
  block *images;
  int *sizes;
  unsigned char *flags;
  FILE *ff;
  bool spritesAreCompressed;
  long cachesize;               // size in bytes of currently cached images
  int *mrulist, *mrubacklink;
  int liststart, listend;
  int lastLoad;
  long maxCacheSize;
  long lockedSize;              // size in bytes of currently locked images

private:
  void compressSprite(block sprite, FILE *ooo);
  bool loadSpriteIndexFile(int expectedFileID, long spr_initial_offs, short numspri);
};

#endif // __SPRCACHE_H
