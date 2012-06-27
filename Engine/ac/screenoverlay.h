
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREENOVERLAY_H
#define __AGS_EE_AC__SCREENOVERLAY_H

#include "ali3d.h"

struct ScreenOverlay {
    IDriverDependantBitmap *bmp;
    block pic;
    int type,x,y,timeout;
    int bgSpeechForChar;
    int associatedOverlayHandle;
    bool hasAlphaChannel;
    bool positionRelativeToScreen;
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
