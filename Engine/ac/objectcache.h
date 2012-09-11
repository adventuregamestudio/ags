
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__OBJECTCACHE_H
#define __AGS_EE_AC__OBJECTCACHE_H

// stores cached object info
struct ObjectCache {
    Common::IBitmap *image;
    int   sppic;
    short tintredwas, tintgrnwas, tintbluwas, tintamntwas, tintlightwas;
    short lightlevwas, mirroredWas, zoomWas;
    // The following are used to determine if the character has moved
    int   xwas, ywas;
};

#endif // __AGS_EE_AC__OBJECTCACHE_H
