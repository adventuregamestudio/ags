
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__SPRITE_H
#define __AGS_EE_AC__SPRITE_H

void get_new_size_for_sprite (int ee, int ww, int hh, int &newwid, int &newhit);
// set any alpha-transparent pixels in the image to the appropriate
// RGB mask value so that the ->Blit calls work correctly
void set_rgb_mask_using_alpha_channel(Common::Bitmap *image);
// from is a 32-bit RGBA image, to is a 15/16/24-bit destination image
Common::Bitmap *remove_alpha_channel(Common::Bitmap *from);
void pre_save_sprite(int ee);
void initialize_sprite (int ee);

#endif // __AGS_EE_AC__SPRITE_H
