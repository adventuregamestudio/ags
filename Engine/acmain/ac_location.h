
int GetRegionAt (int xxx, int yyy);
int get_walkable_area_at_location(int xx, int yy);
int get_walkable_area_at_character (int charnum);
int GetLocationType(int xxx,int yyy);
void GetLocationName(int xxx,int yyy,char*tempo);
int GetGUIAt (int xx,int yy);
int get_hotspot_at(int xpp,int ypp);
int is_pos_on_character(int xx,int yy);
int check_click_on_object(int xx,int yy,int mood);
int __GetLocationType(int xxx,int yyy, int allowHotspot0);


extern int getloctype_index, getloctype_throughgui;



