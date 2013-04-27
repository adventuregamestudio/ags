//
// Implementation from acroom.cpp specific to AGS.Native library
//

#include "util/wgt2allg.h"
#include "game/roominfo.h"

//=============================================================================
// AGS.Native-specific implementation split out of acroom.cpp
//=============================================================================

bool load_room_is_version_bad(AGS::Common::RoomInfo *rstruc)
{
    return ((rstruc->LoadedVersion < kRoomVersion_241) || (rstruc->LoadedVersion > kRoomVersion_Current));
}
