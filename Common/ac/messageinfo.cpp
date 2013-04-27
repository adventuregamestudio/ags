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

#include "ac/messageinfo.h"
#include "util/stream.h"

using AGS::Common::Stream;

MessageInfo::MessageInfo()
    : displayas(0)
    , flags(0)
{
}

void MessageInfo::ReadFromFile(Stream *in)
{
    displayas = in->ReadInt8();
    flags = in->ReadInt8();
}

void MessageInfo::WriteToFile(Stream *out) const
{
     out->WriteInt8(displayas);
     out->WriteInt8(flags);
}
