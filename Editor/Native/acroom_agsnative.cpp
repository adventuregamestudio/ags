//
// Implementation from acroom.cpp specific to AGS.Native library
//

// Headers, as they are in acroom.cpp

#include "util/misc.h"
#include "util/wgt2allg.h"
#include "ac/roomstruct.h"
//

//=============================================================================
// AGS.Native-specific implementation split out of acroom.cpp
//=============================================================================

bool load_room_is_version_bad(roomstruct *rstruc)
{
  return ((rstruc->wasversion < 15) || (rstruc->wasversion > ROOM_FILE_VERSION));
}
