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

using AGS::Common::String;
using AGS::Common::ConfigTree;

extern char *ac_config_file;

void read_config_file(ConfigTree &cfg, const char *alt_cfg_file);
void save_config_file();

bool INIreaditem(const ConfigTree &cfg, const String &sectn, const String &item, String &value);
String INIreadstring(const ConfigTree &cfg, const String &sectn, const String &item, const String &def_value = "");
int INIreadint(const ConfigTree &cfg, const String &sectn, const String &item);
float INIreadfloat(const ConfigTree &cfg, const String &sectn, const String &item, float def_value = 0.f);
void INIwritestring(ConfigTree &cfg, const String &sectn, const String &item, const String &value);


#endif // __AGS_EE_MAIN__CONFIG_H
