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
// Implementation from acroom.cpp specific to Engine runtime
//
//=============================================================================

// Headers, as they are in acroom.cpp

#include "util/misc.h"
#include "ac/roomstruct.h"

//=============================================================================
// Engine-specific implementation split out of acroom.cpp
//=============================================================================

bool load_room_is_version_bad(roomstruct *rstruc)
{
    return ((rstruc->wasversion < kRoomVersion_250b) || (rstruc->wasversion > kRoomVersion_Current));
}
