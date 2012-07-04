
#include <stdio.h>
#include "wgt2allg.h"
#include "ac/ac_object.h"
#include "ac/global_gui.h"
#include "ac/global_inventoryitem.h"
#include "ac/hotspot.h"
#include "acmain/ac_maindefines.h"
#include "acmain/ac_location.h"
#include "acmain/ac_commonheaders.h"
#include "acmain/ac_inventoryitem.h"

int getloctype_index = 0, getloctype_throughgui = 0;

// return the walkable area at the character's feet, taking into account
// that he might just be off the edge of one
int get_walkable_area_at_location(int xx, int yy) {

    int onarea = get_walkable_area_pixel(xx, yy);

    if (onarea < 0) {
        // the character has walked off the edge of the screen, so stop them
        // jumping up to full size when leaving
        if (xx >= thisroom.width)
            onarea = get_walkable_area_pixel(thisroom.width-1, yy);
        else if (xx < 0)
            onarea = get_walkable_area_pixel(0, yy);
        else if (yy >= thisroom.height)
            onarea = get_walkable_area_pixel(xx, thisroom.height - 1);
        else if (yy < 0)
            onarea = get_walkable_area_pixel(xx, 1);
    }
    if (onarea==0) {
        // the path finder sometimes slightly goes into non-walkable areas;
        // so check for scaling in adjacent pixels
        const int TRYGAP=2;
        onarea = get_walkable_area_pixel(xx + TRYGAP, yy);
        if (onarea<=0)
            onarea = get_walkable_area_pixel(xx - TRYGAP, yy);
        if (onarea<=0)
            onarea = get_walkable_area_pixel(xx, yy + TRYGAP);
        if (onarea<=0)
            onarea = get_walkable_area_pixel(xx, yy - TRYGAP);
        if (onarea < 0)
            onarea = 0;
    }

    return onarea;
}

int get_walkable_area_at_character (int charnum) {
    CharacterInfo *chin = &game.chars[charnum];
    return get_walkable_area_at_location(chin->x, chin->y);
}




// X and Y co-ordinates must be in 320x200 format
int check_click_on_object(int xx,int yy,int mood) {
    int aa = GetObjectAt(xx - divide_down_coordinate(offsetx), yy - divide_down_coordinate(offsety));
    if (aa < 0) return 0;
    RunObjectInteraction(aa, mood);
    return 1;
}

extern int obj_lowest_yp;
extern int char_lowest_yp;


// allowHotspot0 defines whether Hotspot 0 returns LOCTYPE_HOTSPOT
// or whether it returns 0
int __GetLocationType(int xxx,int yyy, int allowHotspot0) {
  getloctype_index = 0;
  // If it's not in ProcessClick, then return 0 when over a GUI
  if ((GetGUIAt(xxx, yyy) >= 0) && (getloctype_throughgui == 0))
    return 0;

  getloctype_throughgui = 0;

  xxx += divide_down_coordinate(offsetx);
  yyy += divide_down_coordinate(offsety);
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return 0;

  // check characters, objects and walkbehinds, work out which is
  // foremost visible to the player
  int charat = is_pos_on_character(xxx,yyy);
  int hsat = get_hotspot_at(xxx,yyy);
  int objat = GetObjectAt(xxx - divide_down_coordinate(offsetx), yyy - divide_down_coordinate(offsety));

  multiply_up_coordinates(&xxx, &yyy);

  int wbat = getpixel(thisroom.object, xxx, yyy);

  if (wbat <= 0) wbat = 0;
  else wbat = croom->walkbehind_base[wbat];

  int winner = 0;
  // if it's an Ignore Walkbehinds object, then ignore the walkbehind
  if ((objat >= 0) && ((objs[objat].flags & OBJF_NOWALKBEHINDS) != 0))
    wbat = 0;
  if ((charat >= 0) && ((game.chars[charat].flags & CHF_NOWALKBEHINDS) != 0))
    wbat = 0;
  
  if ((charat >= 0) && (objat >= 0)) {
    if ((wbat > obj_lowest_yp) && (wbat > char_lowest_yp))
      winner = LOCTYPE_HOTSPOT;
    else if (obj_lowest_yp > char_lowest_yp)
      winner = LOCTYPE_OBJ;
    else
      winner = LOCTYPE_CHAR;
  }
  else if (charat >= 0) {
    if (wbat > char_lowest_yp)
      winner = LOCTYPE_HOTSPOT;
    else
      winner = LOCTYPE_CHAR;
  }
  else if (objat >= 0) {
    if (wbat > obj_lowest_yp)
      winner = LOCTYPE_HOTSPOT;
    else
      winner = LOCTYPE_OBJ;
  }

  if (winner == 0) {
    if (hsat >= 0)
      winner = LOCTYPE_HOTSPOT;
  }

  if ((winner == LOCTYPE_HOTSPOT) && (!allowHotspot0) && (hsat == 0))
    winner = 0;

  if (winner == LOCTYPE_HOTSPOT)
    getloctype_index = hsat;
  else if (winner == LOCTYPE_CHAR)
    getloctype_index = charat;
  else if (winner == LOCTYPE_OBJ)
    getloctype_index = objat;

  return winner;
}

// GetLocationType exported function - just call through
// to the main function with default 0
int GetLocationType(int xxx,int yyy) {
  return __GetLocationType(xxx, yyy, 0);
}


void SaveCursorForLocationChange() {
  // update the current location name
  char tempo[100];
  GetLocationName(divide_down_coordinate(mousex), divide_down_coordinate(mousey), tempo);

  if (play.get_loc_name_save_cursor != play.get_loc_name_last_time) {
    play.get_loc_name_save_cursor = play.get_loc_name_last_time;
    play.restore_cursor_mode_to = GetCursorMode();
    play.restore_cursor_image_to = GetMouseCursor();
    DEBUG_CONSOLE("Saving mouse: mode %d cursor %d", play.restore_cursor_mode_to, play.restore_cursor_image_to);
  }
}


void GetLocationName(int xxx,int yyy,char*tempo) {
  if (displayed_room < 0)
    quit("!GetLocationName: no room has been loaded");

  VALIDATE_STRING(tempo);
  
  if (GetGUIAt(xxx, yyy) >= 0) {
    tempo[0]=0;
    int mover = GetInvAt (xxx, yyy);
    if (mover > 0) {
      if (play.get_loc_name_last_time != 1000 + mover)
        guis_need_update = 1;
      play.get_loc_name_last_time = 1000 + mover;
      strcpy(tempo,get_translation(game.invinfo[mover].name));
    }
    else if ((play.get_loc_name_last_time > 1000) && (play.get_loc_name_last_time < 1000 + MAX_INV)) {
      // no longer selecting an item
      guis_need_update = 1;
      play.get_loc_name_last_time = -1;
    }
    return;
  }
  int loctype = GetLocationType (xxx, yyy);
  xxx += divide_down_coordinate(offsetx); 
  yyy += divide_down_coordinate(offsety);
  tempo[0]=0;
  if ((xxx>=thisroom.width) | (xxx<0) | (yyy<0) | (yyy>=thisroom.height))
    return;

  int onhs,aa;
  if (loctype == 0) {
    if (play.get_loc_name_last_time != 0) {
      play.get_loc_name_last_time = 0;
      guis_need_update = 1;
    }
    return;
  }

  // on character
  if (loctype == LOCTYPE_CHAR) {
    onhs = getloctype_index;
    strcpy(tempo,get_translation(game.chars[onhs].name));
    if (play.get_loc_name_last_time != 2000+onhs)
      guis_need_update = 1;
    play.get_loc_name_last_time = 2000+onhs;
    return;
  }
  // on object
  if (loctype == LOCTYPE_OBJ) {
    aa = getloctype_index;
    strcpy(tempo,get_translation(thisroom.objectnames[aa]));
    if (play.get_loc_name_last_time != 3000+aa)
      guis_need_update = 1;
    play.get_loc_name_last_time = 3000+aa;
    return;
  }
  onhs = getloctype_index;
  if (onhs>0) strcpy(tempo,get_translation(thisroom.hotspotnames[onhs]));
  if (play.get_loc_name_last_time != onhs)
    guis_need_update = 1;
  play.get_loc_name_last_time = onhs;
}

const char* Game_GetLocationName(int x, int y) {
  char buffer[STD_BUFFER_SIZE];
  GetLocationName(x, y, buffer);
  return CreateNewScriptString(buffer);
}
