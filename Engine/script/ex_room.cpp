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
// Exporting Room script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_room_script_functions()
{
	ccAddExternalStaticFunction("Room::GetDrawingSurfaceForBackground^1", (void *)Room_GetDrawingSurfaceForBackground);
	ccAddExternalStaticFunction("Room::GetTextProperty^1",(void *)Room_GetTextProperty);
	ccAddExternalStaticFunction("Room::get_BottomEdge", (void *)Room_GetBottomEdge);
	ccAddExternalStaticFunction("Room::get_ColorDepth", (void *)Room_GetColorDepth);
	ccAddExternalStaticFunction("Room::get_Height", (void *)Room_GetHeight);
	ccAddExternalStaticFunction("Room::get_LeftEdge", (void *)Room_GetLeftEdge);
	ccAddExternalStaticFunction("Room::geti_Messages",(void *)Room_GetMessages);
	ccAddExternalStaticFunction("Room::get_MusicOnLoad", (void *)Room_GetMusicOnLoad);
	ccAddExternalStaticFunction("Room::get_ObjectCount", (void *)Room_GetObjectCount);
	ccAddExternalStaticFunction("Room::get_RightEdge", (void *)Room_GetRightEdge);
	ccAddExternalStaticFunction("Room::get_TopEdge", (void *)Room_GetTopEdge);
	ccAddExternalStaticFunction("Room::get_Width", (void *)Room_GetWidth);
}
