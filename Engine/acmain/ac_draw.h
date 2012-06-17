#ifndef __AC_DRAW_H
#define __AC_DRAW_H

#include "ali3d.h"

void invalidate_screen();
void mark_current_background_dirty();
void invalidate_cached_walkbehinds();
void put_sprite_256(int xxx,int yyy,block piccy);

extern color palette[256];

void setpal();

AGS_INLINE int convert_to_low_res(int coord);
AGS_INLINE int convert_back_to_high_res(int coord);

extern IGraphicsDriver *gfxDriver;
extern IDriverDependantBitmap *mouseCursor;
extern IDriverDependantBitmap *blankImage;
extern IDriverDependantBitmap *blankSidebarImage;
extern IDriverDependantBitmap *debugConsole;

extern block *actsps;


#endif // __AC_DRAW_H