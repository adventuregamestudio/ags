#ifndef __AC_WALKABLEAREA_H
#define __AC_WALKABLEAREA_H

void redo_walkable_areas();
int get_walkable_area_pixel(int x, int y);
int get_area_scaling (int onarea, int xx, int yy);
void scale_sprite_size(int sppic, int zoom_level, int *newwidth, int *newheight);
int GetScalingAt (int x, int y);
void SetAreaScaling(int area, int min, int max);
void remove_walkable_areas_from_temp(int fromx, int cwidth, int starty, int endy);
int is_point_in_rect(int x, int y, int left, int top, int right, int bottom);
block prepare_walkable_areas (int sourceChar);
void RemoveWalkableArea(int areanum);
void RestoreWalkableArea(int areanum);
int GetWalkableAreaAt(int xxx,int yyy);

extern block walkareabackup, walkable_areas_temp;

#endif // __AC_WALKABLEAREA_H
