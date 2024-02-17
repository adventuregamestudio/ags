//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__ROOM_H
#define __AGS_EE_AC__ROOM_H

#include "ac/dynobj/scriptdrawingsurface.h"
#include "ac/characterinfo.h"
#include "script/runtimescriptvalue.h"
#include "game/roomstruct.h"

ScriptDrawingSurface* Room_GetDrawingSurfaceForBackground(int backgroundNumber);
ScriptDrawingSurface* Room_GetDrawingSurfaceForMask(RoomAreaMask mask);
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
int Room_GetProperty(const char *property);
const char* Room_GetMessages(int index);
RuntimeScriptValue Sc_Room_GetProperty(const RuntimeScriptValue *params, int32_t param_count);
ScriptDrawingSurface *GetDrawingSurfaceForWalkableArea();
ScriptDrawingSurface *GetDrawingSurfaceForWalkbehind();
ScriptDrawingSurface *Hotspot_GetDrawingSurface();
ScriptDrawingSurface *Region_GetDrawingSurface();

//=============================================================================

void  save_room_data_segment ();
void  unload_old_room();
void  load_new_room(int newnum,CharacterInfo*forchar);
void  new_room(int newnum,CharacterInfo*forchar);
// Sets up a placeholder room object; this is used to avoid occasional crashes
// in case an API function was called that needs to access a room, while no real room is loaded
void  set_room_placeholder();
int   find_highest_room_entered();
void  first_room_initialization();
void  check_new_room();
void  compile_room_script();
void  on_background_frame_change ();
// Clear the current room pointer if room status is no longer valid
void  croom_ptr_clear();

// Following functions convert coordinates between room resolution and region mask.
// Region masks can be 1:N of the room size: 1:1, 1:2 etc.
// In contemporary games this is simply multiplying or dividing on mask resolution.
// In legacy upscale mode (and generally pre-3.* high-res games) things are more
// complicated, as first we need to make an additional conversion between data coords
// and upscale game coordinates.
//
// coordinate conversion      (data) ---> game ---> (room mask)
int room_to_mask_coord(int coord);
// coordinate conversion (room mask) ---> game ---> (data)
int mask_to_room_coord(int coord);

struct MoveList;
// Convert move path from room's mask resolution to room resolution
void convert_move_path_to_room_resolution(MoveList *ml, int from_step = 0, int to_step = -1);

extern AGS::Common::RoomStruct thisroom;

#endif // __AGS_EE_AC__ROOM_H
