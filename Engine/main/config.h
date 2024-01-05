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
#ifndef __AGS_EE_MAIN__CONFIG_H
#define __AGS_EE_MAIN__CONFIG_H

#include "main/graphics_mode.h"
#include "util/ini_util.h"

using AGS::Common::String;
using AGS::Common::ConfigTree;

// Set up default config settings
void config_defaults();
// Find and default configuration file (usually located in the game installation directory)
String find_default_cfg_file();
// Find all-games user configuration file
String find_user_global_cfg_file();
// Find and game-specific user configuration file (located into writable user directory)
String find_user_cfg_file();
// Apply overriding values from the external config (e.g. for mobile ports)
void override_config_ext(ConfigTree &cfg);
// Setup game using final config tree
void apply_config(const ConfigTree &cfg);
// Fixup game setup parameters
void post_config();

void save_config_file();

WindowSetup parse_window_mode(const String &option, bool as_windowed, WindowSetup def_value = WindowSetup());
FrameScaleDef parse_scaling_option(const String &option, FrameScaleDef def_value = kFrame_Undefined);
String make_window_mode_option(const WindowSetup &ws, const Size &game_res, const Size &desktop_res);
String make_scaling_option(FrameScaleDef scale_def);
uint32_t convert_scaling_to_fp(int scale_factor);
int convert_fp_to_scaling(uint32_t scaling);

#endif // __AGS_EE_MAIN__CONFIG_H
