#ifndef __AC_ROOM_H
#define __AC_ROOM_H

void NewRoom(int nrnum);
void unload_old_room() ;
void save_room_data_segment ();
void load_new_room(int newnum,CharacterInfo*forchar);
void first_room_initialization();

#endif // __AC_ROOM_H