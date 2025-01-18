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
#include "gfx/bitmap.h"
#include "util/stream.h"

struct ScriptDrawingSurface final : AGSCCDynamicObject
{
    using Bitmap = AGS::Common::Bitmap;
public:
    // These numbers and types are used to determine the source of this drawing surface;
    // only one of them can be valid for this surface.
    // TODO: revamp this, use shared pointer, or a kind of a locking mechanism,
    // but also invalidate ScriptDrawingSurface when a wrapped bitmap gets unavailable.
    int roomBackgroundNumber;
    RoomAreaMask roomMaskType;
    int dynamicSpriteNumber;
    int dynamicSurfaceNumber;
    bool isLinkedBitmapOnly;
    Bitmap *linkedBitmapOnly;
    int modified;

    int Dispose(void *address, bool force) override;
    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;
    // Tells if DrawingSurface is currently in alpha blending drawing mode
    inline bool IsAlphaBlending() const { return _alphaBlending; }
    int GetRealDrawingColor() const { return _currentColor; }
    int GetScriptDrawingColor() const { return _currentScriptColor; }
    void SetDrawingColor(int color_number);

    // TODO: review this, may need to hide GetBitmapSurface and use StartDrawingReadOnly instead;
    // the reason is that the bitmap may have to be "locked" for the duration of use, as it is
    // not owned by ScriptDrawingSurface. Alternatively, consider using shared ptr,
    // but this may conflict with other things in the engine.
    Bitmap *GetBitmapSurface();
    Bitmap *StartDrawing();
    Bitmap *StartDrawingWithBrush();
    Bitmap *StartDrawingReadOnly();
    void FinishedDrawing();
    void FinishedDrawingWithBrush();
    void FinishedDrawingReadOnly();

    ScriptDrawingSurface();

private:
    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;

    // Drawing mode
    bool _alphaBlending = false;
    int _currentColor = 0;
    int _currentScriptColor = 0;
};

#endif // __AC_SCRIPTDRAWINGSURFACE_H
