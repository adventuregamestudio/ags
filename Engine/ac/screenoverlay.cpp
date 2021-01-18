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
#include "screenoverlay.h"
#include "util/stream.h"

using AGS::Common::Stream;

void ScreenOverlay::ReadFromFile(Stream *in, bool &has_bitmap, int32_t cmp_ver)
{
    pic = nullptr;
    ddb = nullptr;
    in->ReadInt32(); // ddb 32-bit pointer value (nasty legacy format)
    has_bitmap = in->ReadInt32() != 0;
    type = in->ReadInt32();
    x = in->ReadInt32();
    y = in->ReadInt32();
    timeout = in->ReadInt32();
    bgSpeechForChar = in->ReadInt32();
    associatedOverlayHandle = in->ReadInt32();
    hasAlphaChannel = in->ReadBool();
    positionRelativeToScreen = in->ReadBool();
    if (cmp_ver >= 1)
    {
        offsetX = in->ReadInt32();
        offsetY = in->ReadInt32();
    }
    if (cmp_ver >= 2)
    {
        zorder = in->ReadInt32();
        transparency = in->ReadInt32();
        scaleWidth = in->ReadInt32();
        scaleHeight = in->ReadInt32();
    }
}

void ScreenOverlay::WriteToFile(Stream *out) const
{
    out->WriteInt32(0); // ddb 32-bit pointer value (nasty legacy format)
    out->WriteInt32(pic ? 1 : 0); // has bitmap
    out->WriteInt32(type);
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(timeout);
    out->WriteInt32(bgSpeechForChar);
    out->WriteInt32(associatedOverlayHandle);
    out->WriteBool(hasAlphaChannel);
    out->WriteBool(positionRelativeToScreen);
    // since cmp_ver = 1
    out->WriteInt32(offsetX);
    out->WriteInt32(offsetY);
    // since cmp_ver = 2
    out->WriteInt32(zorder);
    out->WriteInt32(transparency);
    out->WriteInt32(scaleWidth);
    out->WriteInt32(scaleHeight);
}
