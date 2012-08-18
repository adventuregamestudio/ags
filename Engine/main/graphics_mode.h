
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
