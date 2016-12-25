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
#ifndef __AGS_EE_MAIN__ENGINE_H
#define __AGS_EE_MAIN__ENGINE_H

const char *get_engine_version();
void		show_preload ();
void		engine_init_game_settings();
int         initialize_engine(int argc,char*argv[]);
int			initialize_engine_with_exception_handling(int argc,char*argv[]);

struct ScreenSetup;
// Try to set new graphics mode deduced from given configuration;
// if requested mode fails, tries to find any compatible mode close to the
// requested one.
bool        engine_try_set_gfxmode_any(const ScreenSetup &setup);
// Tries to switch between fullscreen and windowed mode; uses previously saved
// setup if it is available, or default settings for the new mode
bool        engine_try_switch_windowed_gfxmode();
// Shutdown graphics mode (used before shutting down tha application)
void        engine_shutdown_gfxmode();

extern char *music_file;
extern char *speech_file;

// Register a callback that will be called before engine is initialised.
// Used for apps to register their own plugins and other configuration
typedef void (*t_engine_pre_init_callback)(void);
extern void engine_set_pre_init_callback(t_engine_pre_init_callback callback);

#endif // __AGS_EE_MAIN__ENGINE_H
