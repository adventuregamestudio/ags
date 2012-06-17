
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

