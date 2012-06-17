

block prepare_walkable_areas (int sourceChar);
int get_area_scaling (int onarea, int xx, int yy);
void scale_sprite_size(int sppic, int zoom_level, int *newwidth, int *newheight);
int get_walkable_area_pixel(int x, int y);
void redo_walkable_areas();

extern block walkareabackup, walkable_areas_temp;
