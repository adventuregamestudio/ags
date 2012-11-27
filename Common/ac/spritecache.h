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

#ifndef __SPRCACHE_H
#define __SPRCACHE_H

namespace AGS { namespace Common { class DataStream; } }
using namespace AGS; // FIXME later

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
  void set(int, Common::Bitmap *);
  void setNonDiscardable(int, Common::Bitmap *);
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

  Common::Bitmap *operator[] (int index);

  long *offsets;
  long sprite0InitialOffset;
  long elements;                // size of offsets/images arrays
  Common::Bitmap **images;
  int *sizes;
  unsigned char *flags;
  Common::DataStream *cache_stream;
  bool spritesAreCompressed;
  long cachesize;               // size in bytes of currently cached images
  int *mrulist, *mrubacklink;
  int liststart, listend;
  int lastLoad;
  long maxCacheSize;
  long lockedSize;              // size in bytes of currently locked images

private:
    void compressSprite(Common::Bitmap *sprite, Common::DataStream *out);
  bool loadSpriteIndexFile(int expectedFileID, long spr_initial_offs, short numspri);

  void initFile_adjustBuffers(short numspri);
  void initFile_initNullSpriteParams(int vv);
};

extern SpriteCache spriteset;

#endif // __SPRCACHE_H
