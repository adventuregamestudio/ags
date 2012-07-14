
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SPRITELISTENTRY_H
#define __AGS_EE_AC__SPRITELISTENTRY_H

struct SpriteListEntry {
    IDriverDependantBitmap *bmp;
    block pic;
    int baseline;
    int x,y;
    int transparent;
    bool takesPriorityIfEqual;
    bool hasAlphaChannel;
};

#endif // __AGS_EE_AC__SPRITELISTENTRY_H
