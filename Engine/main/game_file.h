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
//
//
//=============================================================================
#ifndef __AGS_EE_MAIN__GAMEFILE_H
#define __AGS_EE_MAIN__GAMEFILE_H

#include "util/string.h"

void set_default_glmsg (int msgnum, const char* val);

extern void ReadGameSetupStructBase_Aligned(Common::Stream *in);
extern void WriteGameSetupStructBase_Aligned(Common::Stream *out);

extern AGS::Common::String game_file_name;

void init_script_invitems(int max_invitems);
void init_script_dialogs(int max_invitems);

#endif // __AGS_EE_MAIN__GAMEFILE_H
