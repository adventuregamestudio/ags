
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SPRITE_H
#define __AGS_EE_AC__SPRITE_H

void get_new_size_for_sprite (int ee, int ww, int hh, int &newwid, int &newhit);
// set any alpha-transparent pixels in the image to the appropriate
// RGB mask value so that the draw_sprite calls work correctly
void set_rgb_mask_using_alpha_channel(block image);
// from is a 32-bit RGBA image, to is a 15/16/24-bit destination image
block remove_alpha_channel(block from);
void pre_save_sprite(int ee);
void initialize_sprite (int ee);

#endif // __AGS_EE_AC__SPRITE_H
