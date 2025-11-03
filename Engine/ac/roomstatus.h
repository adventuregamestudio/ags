//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#ifndef __AGS_EE_AC__ROOMSTATUS_H
#define __AGS_EE_AC__ROOMSTATUS_H

#include "ac/roomobject.h"
#include "game/scripteventtable.h"
#include "game/roomstruct.h"
#include "game/savegame.h"
#include "util/string_types.h"

// Forward declaration
namespace AGS { namespace Common { class Stream; } }
using AGS::Common::Stream;

// RoomStatus runtime save format
enum RoomStatSvgVersion
{
    kRoomStatSvgVersion_Initial  = 0, // [UNSUPPORTED] from 3.5.0 pre-alpha
    // NOTE: in 3.5.0 "Room States" had lower index than "Loaded Room State" by mistake
    kRoomStatSvgVersion_350_Mismatch = 0, // an incorrect "Room States" version from 3.5.0
    kRoomStatSvgVersion_350      = 1, // new movelist format (along with pathfinder)
    kRoomStatSvgVersion_36016    = 2, // hotspot and object names
    kRoomStatSvgVersion_36025    = 3, // object animation volume
    kRoomStatSvgVersion_36041    = 4, // room state's contentFormat
    kRoomStatSvgVersion_36109    = 5, // removed movelists, save externally
    kRoomStatSvgVersion_36214    = 3060214, // serialize current room's changes to masks
    kRoomStatSvgVersion_400      = 4000000, // room object blendmodes etc
    kRoomStatSvgVersion_40003    = 4000003, // room object flags as 32-bit, facedirratio
    kRoomStatSvgVersion_40008    = 4000008, // custom properties for regions and walk-areas
    kRoomStatSvgVersion_40016    = 4000016, // room objects expose script motion path
    kRoomStatSvgVersion_40018    = 4000018, // shaders
    kRoomStatSvgVersion_40020    = 4000020, // expanded and bit more consistent anim params serialization
    kRoomStatSvgVersion_40023    = 4000023, // sync with kRoomStatSvgVersion_36214
    kRoomStatSvgVersion_Current  = kRoomStatSvgVersion_40023
};

struct HotspotState
{
    bool Enabled = false;
    Common::String Name;

    void ReadFromSavegame(Common::Stream *in, RoomStatSvgVersion save_ver);
    void WriteToSavegame(Common::Stream *out) const;
};

struct WalkareaState
{
    float FaceDirectionRatio = 0.f;

    void ReadFromSavegame(Common::Stream *in, RoomStatSvgVersion save_ver);
    void WriteToSavegame(Common::Stream *out) const;
};

// RoomStatus contains everything about a room that could change at runtime.
class RoomStatus
{
public:
    // TODO: hide these fields under get/set methods
    int   beenhere = 0;
    uint32_t numobj = 0;
    std::vector<RoomObject> obj;
    uint32_t tsdatasize = 0;
    std::vector<uint8_t> tsdata;

    HotspotState hotspot[MAX_ROOM_HOTSPOTS];
    char  region_enabled[MAX_ROOM_REGIONS];
    short walkbehind_base[MAX_WALK_BEHINDS];
    float face_dir_ratio = 0.f;
    WalkareaState walkareas[MAX_WALK_AREAS];

    Common::StringIMap roomProps;
    std::vector<Common::StringIMap> objProps;
    Common::StringIMap hsProps[MAX_ROOM_HOTSPOTS];
    Common::StringIMap regProps[MAX_ROOM_REGIONS];
    Common::StringIMap waProps[MAX_WALK_AREAS];

    // A version of a save this RoomStatus was restored from.
    // This is used as a hint when merging RoomStatus with the loaded room file (upon room enter).
    // We need this for cases when an old format save is restored within an upgraded game
    // (for example, game was upgraded from 3.4.0 to 3.6.0, but player tries loading 3.4.0 save),
    // because room files are only loaded once entered, so we cannot fixup all RoomStatuses at once.
    RoomStatSvgVersion contentFormat = kRoomStatSvgVersion_Current;

    RoomStatus();
    ~RoomStatus();

    int  GetBgShaderID() const { return _bgShaderID; }
    int  GetBgShaderHandle() const { return _bgShaderHandle; }
    void SetBgShader(int shader_id, int shader_handle);

    void ReadFromSavegame(Common::Stream *in, RoomStatSvgVersion cmp_ver);
    void WriteToSavegame(Common::Stream *out) const;

private:
    void FreeScriptData();
    void FreeProperties();

    // TODO: a RAII wrapper over managed handle, that auto releases the reference
    int _bgShaderID = 0;
    int _bgShaderHandle = 0;
};

// Retrieves a given room's runtime state. *Allocates* one if it does not exist
RoomStatus* GetRoomState(int room);
// Retrieves a existing room's runtime state; returns null if one was not allocated
RoomStatus *GetRoomStateIfExists(int room);
// Checks whether the room state exists.
bool IsRoomStateValid(int room);
// Deletes particular room's runtime state. This will make the game forget about
// what happened in that room, and make it start anew whenever player enters it again.
void ResetRoomState(int room);
// Deletes all room runtime states.
void ResetRoomStates();

#endif // __AGS_EE_AC__ROOMSTATUS_H
