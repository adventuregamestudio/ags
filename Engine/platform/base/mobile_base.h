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
#include "core/platform.h"

#if (AGS_PLATFORM_OS_ANDROID) || (AGS_PLATFORM_OS_IOS)

// Writes mobile platform config
bool WriteConfiguration(const char *filename);
// Reads mobile platform config
bool ReadConfiguration(const char *filename, bool read_everything);
// Reset config variables
void ResetConfiguration();


// Mobile platform options
extern int psp_ignore_acsetup_cfg_file;
extern int psp_clear_cache_on_room_change;
extern int psp_rotation;
extern int psp_config_enabled;
extern char psp_translation[100];
extern char* psp_translations[100];

// Mouse option
extern int config_mouse_control_mode;

// Graphic options
extern int psp_gfx_scaling;
extern int psp_gfx_smoothing;

// Audio options from the Allegro library.
extern unsigned int psp_audio_samplerate;
extern int psp_audio_enabled;
extern int psp_audio_multithreaded;
extern int psp_audio_cachesize;
extern int psp_midi_enabled;
extern int psp_midi_preload_patches;

extern int psp_video_framedrop;

extern int psp_gfx_renderer;
extern int psp_gfx_super_sampling;
extern int psp_gfx_smooth_sprites;

extern int psp_debug_write_to_logcat;

extern int config_mouse_longclick;

extern int display_fps;

#endif
