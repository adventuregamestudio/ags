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
#include "ac/dynobj/cc_guicontrol.h"

using namespace AGS::Common;

// return the type name of the object
const char *CCGUIControl::GetType()
{
    return "GUIControl";
}

size_t CCGUIControl::CalcSerializeSize(const void* /*address*/)
{
    return 0;
}

void CCGUIControl::Serialize(const void* /*address*/, Stream* /*out*/)
{
    // no longer supported
}

void CCGUIControl::Unserialize(int /*index*/, Stream* /*in*/, size_t /*data_sz*/)
{
    // no longer supported
}
