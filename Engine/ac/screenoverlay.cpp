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

void ScreenOverlay::ReadFromFile(Stream *in)
{
    // Skipping bmp and pic pointer values
    in->ReadInt32();
    in->ReadInt32();
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
    out->WriteInt32(0);
    out->WriteInt32(type);
    out->WriteInt32(x);
    out->WriteInt32(y);
    out->WriteInt32(timeout);
    out->WriteInt32(bgSpeechForChar);
    out->WriteInt32(associatedOverlayHandle);
    out->WriteBool(hasAlphaChannel);
    out->WriteBool(positionRelativeToScreen);
}
