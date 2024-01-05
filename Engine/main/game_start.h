//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_MAIN__GAMESTART_H
#define __AGS_EE_MAIN__GAMESTART_H

#include "util/string.h"

void start_game();
void initialize_start_and_play_game(int override_start_room, const AGS::Common::String &load_save);

#endif // __AGS_EE_MAIN__GAMESTART_H
