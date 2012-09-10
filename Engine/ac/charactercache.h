
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__CHARACTERCACHE_H
#define __AGS_EE_AC__CHARACTERCACHE_H

// stores cached info about the character
struct CharacterCache {
    Common::IBitmap *image;
    int sppic;
    int scaling;
    int inUse;
    short tintredwas, tintgrnwas, tintbluwas, tintamntwas;
    short lightlevwas, tintlightwas;
    // no mirroredWas is required, since the code inverts the sprite number
};

#endif // __AGS_EE_AC__CHARACTERCACHE_H
