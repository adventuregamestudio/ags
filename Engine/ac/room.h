//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
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

namespace AGS { namespace Common { class RoomInfo; } }

Common::Bitmap *fix_bitmap_size(Common::Bitmap *todubl);
void  save_room_data_segment ();
void  unload_old_room();
void  convert_room_coordinates_to_low_res(AGS::Common::RoomInfo &room_base);
void  load_new_room(int newnum,CharacterInfo*forchar);
void  new_room(int newnum,CharacterInfo*forchar);
int   find_highest_room_entered();
void  first_room_initialization();
void  check_new_room();
void  compile_room_script();
void  on_background_frame_change ();

#endif // __AGS_EE_AC__ROOM_H
