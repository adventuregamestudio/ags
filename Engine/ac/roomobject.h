
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__ROOMOBJECT_H
#define __AGS_EE_AC__ROOMOBJECT_H

#include "platform/file.h"

// This struct is only used in save games and by plugins
struct RoomObject {
    int   x,y;
    int   transparent;    // current transparency setting
    short tint_r, tint_g;   // specific object tint
    short tint_b, tint_level;
    short tint_light;
    short last_zoom;      // zoom level last time
    short last_width, last_height;   // width/height last time drawn
    short num;            // sprite slot number
    short baseline;       // <=0 to use Y co-ordinate; >0 for specific baseline
    short view,loop,frame; // only used to track animation - 'num' holds the current sprite
    short wait,moving;
    char  cycling;        // is it currently animating?
    char  overall_speed;
    char  on;
    char  flags;
    short blocking_width, blocking_height;

    int get_width();
    int get_height();
    int get_baseline();

    void ReadFromFile(FILE *fp);
    void WriteToFile(FILE *fp);
};

#endif // __AGS_EE_AC__ROOMOBJECT_H
