
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREENOVERLAY_H
#define __AGS_EE_AC__SCREENOVERLAY_H

#include "gfx/ali3d.h"
#include "util/file.h"

// Forward declaration
namespace AGS { namespace Common { class CDataStream; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS::Engine; // FIXME later


struct ScreenOverlay {
    IDriverDependantBitmap *bmp;
    Common::IBitmap *pic;
    int type,x,y,timeout;
    int bgSpeechForChar;
    int associatedOverlayHandle;
    bool hasAlphaChannel;
    bool positionRelativeToScreen;

    void ReadFromFile(Common::CDataStream *in);
    void WriteToFile(Common::CDataStream *out);
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
