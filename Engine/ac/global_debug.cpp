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

#include "ac/global_debug.h"
#include "ac/common.h"
#include "ac/characterinfo.h"
#include "ac/draw.h"
#include "ac/game.h"
#include "ac/gamesetup.h"
#include "ac/gamesetupstruct.h"
#include "ac/gamestate.h"
#include "ac/global_character.h"
#include "ac/global_display.h"
#include "ac/global_room.h"
#include "ac/properties.h"
#include "ac/sys_events.h"
#include "ac/translation.h"
#include "ac/walkablearea.h"
#include "gfx/gfxfilter.h"
#include "gui/guidialog.h"
#include "script/cc_common.h"
#include "debug/debug_log.h"
#include "debug/debugger.h"
#include "main/main.h"
#include "ac/spritecache.h"
#include "gfx/bitmap.h"
#include "gfx/graphicsdriver.h"
#include "main/graphics_mode.h"

using namespace AGS::Common;
using namespace AGS::Engine;

extern GameSetupStruct game;
extern GameSetup usetup;
extern GameState play;
extern RoomStruct thisroom;
extern CharacterInfo*playerchar;

extern IGraphicsDriver *gfxDriver;
extern SpriteCache spriteset;
extern int displayed_room, starting_room;

RoomAreaMask debugLastRoomMask = kRoomAreaNone;
int debugLastMoveChar = -1;

String GetRuntimeInfo()
{
    DisplayMode mode = gfxDriver->GetDisplayMode();
    Rect render_frame = gfxDriver->GetRenderDestination();
    PGfxFilter filter = gfxDriver->GetGraphicsFilter();
    String runtimeInfo = String::FromFormat(
        "Adventure Game Studio run-time engine[ACI version %s"
        "[Game resolution %d x %d (%d-bit)"
        "[Running %d x %d at %d-bit%s[GFX: %s; %s[Draw frame %d x %d["
        "Sprite cache size: %d KB (limit %d KB; %d locked)",
        EngineVersion.LongString.GetCStr(), game.GetGameRes().Width, game.GetGameRes().Height, game.GetColorDepth(),
        mode.Width, mode.Height, mode.ColorDepth,
        (mode.IsWindowed() ? " W" : (mode.IsRealFullscreen() ? " F" : " FD")),
        gfxDriver->GetDriverName(), filter->GetInfo().Name.GetCStr(),
        render_frame.GetWidth(), render_frame.GetHeight(),
        spriteset.GetCacheSize() / 1024, spriteset.GetMaxCacheSize() / 1024, spriteset.GetLockedSize() / 1024);
    if (play.separate_music_lib)
        runtimeInfo.Append("[AUDIO.VOX enabled");
    if (play.voice_avail)
        runtimeInfo.Append("[SPEECH.VOX enabled");
    if (get_translation_tree().size() > 0) {
        runtimeInfo.Append("[Using translation ");
        runtimeInfo.Append(get_translation_name());
    }

    return runtimeInfo;
}

void script_debug(int cmdd,int dataa) {
    if (play.debug_mode==0) return;
    int rr;
    if (cmdd==0) {
        for (rr=1;rr<game.numinvitems;rr++)
            playerchar->inv[rr]=1;
        update_invorder();
    }
    else if (cmdd==1) {
        String toDisplay = GetRuntimeInfo();
        Display(toDisplay.GetCStr());
    }
    else if (cmdd==2) 
    {  // show room mask
        if (loaded_game_file_version < kGameVersion_360) dataa = kRoomAreaWalkable;

        auto mask = static_cast<RoomAreaMask>(dataa);
        if ((mask < kRoomAreaNone) || (mask > kRoomAreaRegion) ||
            (debugLastRoomMask == mask)) // also act like toggling last mask off
            debugLastRoomMask = kRoomAreaNone;
        else
            debugLastRoomMask = mask;
        debug_draw_room_mask(debugLastRoomMask);
    }
    else if (cmdd==3) 
    {
        int goToRoom = -1;
        if (game.roomCount == 0)
        {
            char inroomtex[80];
            snprintf(inroomtex, sizeof(inroomtex), "!Enter new room: (in room %d)", displayed_room);
            setup_for_dialog();
            goToRoom = enternumberwindow(inroomtex);
            restore_after_dialog();
        }
        else
        {
            setup_for_dialog();
            goToRoom = roomSelectorWindow(displayed_room, game.roomCount, game.roomNumbers, game.roomNames);
            restore_after_dialog();
        }
        if (goToRoom >= 0) 
            NewRoom(goToRoom);
    }
    else if (cmdd == 4) {
        if (display_fps != kFPS_Forced)
            display_fps = (FPSDisplayMode)dataa;
    }
    else if (cmdd == 5) {
        // show the given character's pathfinding; act like a on/off toggle
        debugLastMoveChar = dataa == debugLastMoveChar ? -1 : dataa;
        debug_draw_movelist(dataa);
    }
    else if (cmdd == 99)
        ccSetOption(SCOPT_DEBUGRUN, dataa);
    else quit("!Debug: unknown command code");
}
