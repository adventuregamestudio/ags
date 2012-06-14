/*
  Adventure Game Studio source code Copyright 1999-2011 Chris Jones.
  All rights reserved.

  The AGS Editor Source Code is provided under the Artistic License 2.0
  http://www.opensource.org/licenses/artistic-license-2.0.php

  You MAY NOT compile your own builds of the engine without making it EXPLICITLY
  CLEAR that the code has been altered from the Standard Version.

*/
#pragma unmanaged
//#include "wgt2allg_nofunc.h"
#include "wgt2allg.h"
//#include "acroom_nofunc.h"
#include "ac/ac_common.h"   // quit()
#include "sprcache.h"
#include "acruntim.h"
#include "acgui.h"
#include <ctype.h>

#include "bigend.h"
#include "cs/cs_utils.h"				// fputstring, etc
#include "acfont/ac_agsfontrenderer.h"	// fontRenderers



#error 'acgui.cpp' contents were split up to separate modules; do not include this file in the build


extern int get_adjusted_spritewidth(int spr);
extern int get_adjusted_spriteheight(int spr);
extern bool is_sprite_alpha(int spr);
extern int final_col_dep;



extern SpriteCache spriteset;
extern void draw_sprite_compensate(int spr, int x, int y, int xray);
extern inline int divide_down_coordinate(int coord);
extern inline int multiply_up_coordinate(int coord);
extern inline void multiply_up_coordinates(int *x, int *y);
extern inline int get_fixed_pixel_size(int pixels);





extern int wgettextwidth_compensate(const char *tex, int font);






