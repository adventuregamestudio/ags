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
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SCREENOVERLAY_H
#define __AGS_EE_AC__SCREENOVERLAY_H

#include "gfx/ali3d.h"
#include "util/file.h"

// Forward declaration
namespace AGS { namespace Common { class DataStream; } }
namespace AGS { namespace Engine { class IDriverDependantBitmap; }}
using namespace AGS::Engine; // FIXME later


struct ScreenOverlay {
    IDriverDependantBitmap *bmp;
    Common::Bitmap *pic;
    int type,x,y,timeout;
    int bgSpeechForChar;
    int associatedOverlayHandle;
    bool hasAlphaChannel;
    bool positionRelativeToScreen;

    void ReadFromFile(Common::DataStream *in);
    void WriteToFile(Common::DataStream *out);
};

#endif // __AGS_EE_AC__SCREENOVERLAY_H
