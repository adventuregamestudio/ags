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
#ifndef __AGS_EE_MAIN__GRAPHICSMODE_H
#define __AGS_EE_MAIN__GRAPHICSMODE_H

void engine_init_screen_settings();
int  initialize_graphics_filter(const char *filterID, int width, int height, int colDepth);
int  engine_init_gfx_filters();
void create_gfx_driver();
int  init_gfx_mode(int wid,int hit,int cdep);
int  try_widescreen_bordered_graphics_mode_if_appropriate(int initasx, int initasy, int firstDepth);
int  switch_to_graphics_mode(int initasx, int initasy, int scrnwid, int scrnhit, int firstDepth, int secondDepth);
void engine_init_gfx_driver();
int  engine_init_graphics_mode();
void CreateBlankImage();
void engine_post_init_gfx_driver();
void engine_prepare_screen();
void engine_set_gfx_driver_callbacks();
void engine_set_color_conversions();

// set to 0 once successful
extern int working_gfx_mode_status;
extern int debug_15bit_mode, debug_24bit_mode;
extern int convert_16bit_bgr;

#endif // __AGS_EE_MAIN__GRAPHICSMODE_H
