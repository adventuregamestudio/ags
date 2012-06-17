#ifndef __AC_DRAW_H
#define __AC_DRAW_H

void invalidate_screen();
void mark_current_background_dirty();
void invalidate_cached_walkbehinds();

extern color palette[256];

void setpal();

AGS_INLINE int convert_to_low_res(int coord);
AGS_INLINE int convert_back_to_high_res(int coord);


#endif // __AC_DRAW_H