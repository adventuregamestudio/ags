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

using namespace AGS::Common;

void ScreenOverlay::ReadFromFile(Stream *in, bool &has_bitmap, int32_t cmp_ver)
{
    // TODO: find out if it's safe to just drop these pointers!! replace with unique_ptr?
    bmp = nullptr;
    pic = nullptr;
    in->ReadInt32(); // bmp 32-bit pointer value (nasty legacy format)
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
        _offsetX = in->ReadInt32();
        _offsetY = in->ReadInt32();
    }
    if (cmp_ver >= 10)
    {
        blendMode = (BlendMode)in->ReadInt32();
        // Reserved for colour options
        in->ReadInt32(); // colour flags
        in->ReadInt32(); // transparency / alpha
        in->ReadInt32(); // tint rgb + s
        in->ReadInt32(); // tint light (or light level)
        // Reserved for transform options
        in->ReadInt32(); // sprite transform flags1
        in->ReadInt32(); // sprite transform flags2
        in->ReadInt32(); // transform scale x
        in->ReadInt32(); // transform scale y
        in->ReadInt32(); // transform skew x
        in->ReadInt32(); // transform skew y
        in->ReadInt32(); // transform rotate
        in->ReadInt32(); // sprite pivot x
        in->ReadInt32(); // sprite pivot y
        in->ReadInt32(); // sprite anchor x
        in->ReadInt32(); // sprite anchor y
    }
}

void ScreenOverlay::WriteToFile(Stream *out) const
{
    out->WriteInt32(0); // bmp 32-bit pointer value (nasty legacy format)
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
    out->WriteInt32(_offsetX);
    out->WriteInt32(_offsetY);
    // since cmp_ver = 10
    out->WriteInt32(blendMode);
    // Reserved for colour options
    out->WriteInt32(0); // colour flags
    out->WriteInt32(0); // tint rgb + s
    out->WriteInt32(0); // tint light (or light level)
    // Reserved for transform options
    out->WriteInt32(0); // sprite transform flags1
    out->WriteInt32(0); // sprite transform flags2
    out->WriteInt32(0); // transform scale x
    out->WriteInt32(0); // transform scale y
    out->WriteInt32(0); // transform skew x
    out->WriteInt32(0); // transform skew y
    out->WriteInt32(0); // transform rotate
    out->WriteInt32(0); // sprite pivot x
    out->WriteInt32(0); // sprite pivot y
    out->WriteInt32(0); // sprite anchor x
    out->WriteInt32(0); // sprite anchor y
}
