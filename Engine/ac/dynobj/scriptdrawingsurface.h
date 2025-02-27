//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#ifndef __AC_SCRIPTDRAWINGSURFACE_H
#define __AC_SCRIPTDRAWINGSURFACE_H

#include "ac/dynobj/cc_agsdynamicobject.h"
#include "game/roomstruct.h"

namespace AGS { namespace Common { class Bitmap; }}

struct ScriptDrawingSurface final : AGSCCDynamicObject {
    // These numbers and types are used to determine the source of this drawing surface;
    // only one of them can be valid for this surface.
    int roomBackgroundNumber;
    RoomAreaMask roomMaskType;
    int dynamicSpriteNumber;
    int dynamicSurfaceNumber;
    bool isLinkedBitmapOnly;
    Common::Bitmap *linkedBitmapOnly;
    int currentColour;
    int currentColourScript;
    int highResCoordinates;
    int modified;
    int hasAlphaChannel;
    //Common::Bitmap* abufBackup;

    int Dispose(const char *address, bool force) override;
    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;
    Common::Bitmap* GetBitmapSurface();
    Common::Bitmap *StartDrawing();
    void PointToGameResolution(int *xcoord, int *ycoord);
    void SizeToGameResolution(int *width, int *height);
    void SizeToGameResolution(int *adjustValue);
    void SizeToDataResolution(int *adjustValue);
    void FinishedDrawing();
    void FinishedDrawingReadOnly();

    ScriptDrawingSurface();

protected:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize() override;
    // Write object data into the provided stream
    void Serialize(const char *address, AGS::Common::Stream *out) override;
};

#endif // __AC_SCRIPTDRAWINGSURFACE_H