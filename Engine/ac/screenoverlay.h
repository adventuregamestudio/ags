
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREENOVERLAY_H
#define __AGS_EE_AC__SCREENOVERLAY_H

#include "gfx/ali3d.h"
#include "util/file.h"

struct ScreenOverlay {
    IDriverDependantBitmap *bmp;
    block pic;
    int type,x,y,timeout;
    int bgSpeechForChar;
    int associatedOverlayHandle;
    bool hasAlphaChannel;
    bool positionRelativeToScreen;

    void ReadFromFile(FILE *f);
    void WriteToFile(FILE *f);
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
