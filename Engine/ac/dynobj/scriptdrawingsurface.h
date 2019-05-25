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

#ifndef __AC_SCRIPTDRAWINGSURFACE_H
#define __AC_SCRIPTDRAWINGSURFACE_H

#include "ac/dynobj/cc_agsdynamicobject.h"

namespace AGS { namespace Common { class Bitmap; }}

struct ScriptDrawingSurface final : AGSCCDynamicObject {
    int roomBackgroundNumber;
    int dynamicSpriteNumber;
    int dynamicSurfaceNumber;
    bool isLinkedBitmapOnly;
    Common::Bitmap *linkedBitmapOnly;
    int currentColour;
    int currentColourScript;
    int modified;
    int hasAlphaChannel;
    //Common::Bitmap* abufBackup;

    int Dispose(const char *address, bool force) override;
    const char *GetType() override;
    int Serialize(const char *address, char *buffer, int bufsize) override;
    void Unserialize(int index, const char *serializedData, int dataSize) override;
    Common::Bitmap* GetBitmapSurface();
    Common::Bitmap *StartDrawing();
    void FinishedDrawing();
    void FinishedDrawingReadOnly();

    ScriptDrawingSurface();
};

#endif // __AC_SCRIPTDRAWINGSURFACE_H