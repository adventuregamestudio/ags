
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__ROOM_H
#define __AGS_EE_AC__ROOM_H

#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/characterinfo.h"
#include "ac/roomstruct.h"

ScriptDrawingSurface* Room_GetDrawingSurfaceForBackground(int backgroundNumber);
int Room_GetObjectCount();
int Room_GetWidth();
int Room_GetHeight();
int Room_GetColorDepth();
int Room_GetLeftEdge();
int Room_GetRightEdge();
int Room_GetTopEdge();
int Room_GetBottomEdge();
int Room_GetMusicOnLoad();
const char* Room_GetTextProperty(const char *property);
const char* Room_GetMessages(int index);

//=============================================================================

Common::Bitmap *fix_bitmap_size(Common::Bitmap *todubl);
void  save_room_data_segment ();
void  unload_old_room();
void  convert_room_coordinates_to_low_res(roomstruct *rstruc);
void  load_new_room(int newnum,CharacterInfo*forchar);
void  new_room(int newnum,CharacterInfo*forchar);
int   find_highest_room_entered();
void  first_room_initialization();
void  check_new_room();
void  compile_room_script();
void  on_background_frame_change ();

#endif // __AGS_EE_AC__ROOM_H
