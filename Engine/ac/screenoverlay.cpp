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

#include "ac/screenoverlay.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "util/stream.h"

using AGS::Common::Stream;

extern IGraphicsDriver *gfxDriver;

ScreenOverlay::ScreenOverlay()
    : pic(NULL)
    , bmp(NULL)
    , hasSerializedBitmap(false)
{
}

ScreenOverlay::ScreenOverlay(const ScreenOverlay &overlay)
    : pic(NULL)
    , bmp(NULL)
    , hasSerializedBitmap(false)
{
    *this = overlay;
}

ScreenOverlay::~ScreenOverlay()
{
    Free();
}

void ScreenOverlay::Free()
{
    delete pic;
    if (bmp)
    {
        gfxDriver->DestroyDDB(bmp);
    }
    pic = NULL;
    bmp = NULL;
    hasSerializedBitmap = false;
}

void ScreenOverlay::ReadFromFile(Stream *in)
{
    // Skipping bmp and pic pointer values
    in->ReadInt32();
    hasSerializedBitmap = in->ReadInt32() != 0;
    type = in->ReadInt32();
    x = in->ReadInt32();
    y = in->ReadInt32();
    timeout = in->ReadInt32();
    bgSpeechForChar = in->ReadInt32();
    associatedOverlayHandle = in->ReadInt32();
    hasAlphaChannel = in->ReadBool();
    positionRelativeToScreen = in->ReadBool();
}

void ScreenOverlay::WriteToFile(Stream *out)
{
    // Writing bitmap "pointers" to correspond to full structure writing
    out->WriteInt32(0);
    out->WriteInt32(pic ? 1 : 0);
    out->WriteInt32(type);
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(timeout);
    out->WriteInt32(bgSpeechForChar);
    out->WriteInt32(associatedOverlayHandle);
    out->WriteBool(hasAlphaChannel);
    out->WriteBool(positionRelativeToScreen);
}

ScreenOverlay &ScreenOverlay::operator=(const ScreenOverlay &overlay)
{
    Free();
    bmp = overlay.bmp ? gfxDriver->CreateDDBReference(overlay.bmp) : NULL;
    pic = overlay.pic ? Common::BitmapHelper::CreateBitmapReference(overlay.pic) : NULL;
    type = overlay.type;
    x = overlay.x;
    y = overlay.y;
    timeout = overlay.timeout;
    bgSpeechForChar = overlay.bgSpeechForChar;
    associatedOverlayHandle = overlay.associatedOverlayHandle;
    hasAlphaChannel = overlay.hasAlphaChannel;
    positionRelativeToScreen = overlay.positionRelativeToScreen;
    hasSerializedBitmap = overlay.hasSerializedBitmap;
    return *this;
}
