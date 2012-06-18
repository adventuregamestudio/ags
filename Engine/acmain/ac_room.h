#ifndef __AC_ROOM_H
#define __AC_ROOM_H

#include "ac/ac_characterinfo.h"
#include "acrun/ac_scriptdrawingsurface.h"
#include "ac/ac_roomstruct.h"

block fix_bitmap_size(block todubl);
void SetAmbientTint (int red, int green, int blue, int opacity, int luminance);
void save_room_data_segment ();
void unload_old_room();
void convert_room_coordinates_to_low_res(roomstruct *rstruc);
void load_new_room(int newnum,CharacterInfo*forchar);
void new_room(int newnum,CharacterInfo*forchar);
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
void NewRoom(int nrnum);
void NewRoomEx(int nrnum,int newx,int newy);
void NewRoomNPC(int charid, int nrnum, int newx, int newy);
void ResetRoom(int nrnum);
int HasPlayerBeenInRoom(int roomnum);
void CallRoomScript (int value);
int HasBeenToRoom (int roomnum);
int find_highest_room_entered();
void first_room_initialization();
void check_new_room();
void compile_room_script();

#endif // __AC_ROOM_H