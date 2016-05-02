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
#ifndef __AGS_EE_MAIN__CONFIG_H
#define __AGS_EE_MAIN__CONFIG_H

#include "util/ini_util.h"

void read_config_file(char *argv0);
uint32_t parse_scaling_factor(const AGS::Common::String &scaling_option);

bool INIreaditem(const AGS::Common::ConfigTree &cfg, const AGS::Common::String &sectn, const AGS::Common::String &item, AGS::Common::String &value);
int INIreadint(const AGS::Common::ConfigTree &cfg, const AGS::Common::String &sectn, const AGS::Common::String &item, int def_value = 0);
AGS::Common::String INIreadstring(const AGS::Common::ConfigTree &cfg, const AGS::Common::String &sectn, const AGS::Common::String &item, const char *def_value = "");


#endif // __AGS_EE_MAIN__CONFIG_H
