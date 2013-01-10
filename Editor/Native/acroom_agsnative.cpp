//
// Implementation from acroom.cpp specific to AGS.Native library
//

#include "util/wgt2allg.h"
#include "ac/roomstruct.h"

//=============================================================================
// AGS.Native-specific implementation split out of acroom.cpp
//=============================================================================

bool load_room_is_version_bad(roomstruct *rstruc)
{
    return ((rstruc->wasversion < kRoomVersion_241) || (rstruc->wasversion > kRoomVersion_Current));
}
