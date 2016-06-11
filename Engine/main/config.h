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

extern String ac_config_file;
// Find and load default configuration file (usually located in the game installation directory)
void load_default_config_file(AGS::Common::ConfigTree &cfg, const char *alt_cfg_file);
// Find and load user configuration file (located into writable user location)
void load_user_config_file(AGS::Common::ConfigTree &cfg);
// Read optional data file name and location from config
void read_game_data_location(const AGS::Common::ConfigTree &cfg);
// Setup game using final config tree
void read_config(const AGS::Common::ConfigTree &cfg);
// Fixup game setup parameters
void post_config();

void save_config_file();

bool INIreaditem(const ConfigTree &cfg, const String &sectn, const String &item, String &value);
String INIreadstring(const ConfigTree &cfg, const String &sectn, const String &item, const String &def_value = "");
int INIreadint(const ConfigTree &cfg, const String &sectn, const String &item);
float INIreadfloat(const ConfigTree &cfg, const String &sectn, const String &item, float def_value = 0.f);
void INIwritestring(ConfigTree &cfg, const String &sectn, const String &item, const String &value);
void INIwriteint(ConfigTree &cfg, const String &sectn, const String &item, int value);


#endif // __AGS_EE_MAIN__CONFIG_H
