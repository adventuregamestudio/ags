
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__WALKABLEAREA_H
#define __AGS_EE_AC__WALKABLEAREA_H

void  redo_walkable_areas();
int   get_walkable_area_pixel(int x, int y);
int   get_area_scaling (int onarea, int xx, int yy);
void  scale_sprite_size(int sppic, int zoom_level, int *newwidth, int *newheight);
void  remove_walkable_areas_from_temp(int fromx, int cwidth, int starty, int endy);
int   is_point_in_rect(int x, int y, int left, int top, int right, int bottom);
Common::Bitmap *prepare_walkable_areas (int sourceChar);
int   get_walkable_area_at_location(int xx, int yy);
int   get_walkable_area_at_character (int charnum);

#endif // __AGS_EE_AC__WALKABLEAREA_H
