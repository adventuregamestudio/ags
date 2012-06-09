//
// Implementation from acroom.cpp specific to Engine runtime
//

// Headers, as they are in acroom.cpp

// MACPORT FIX: endian support
#include "bigend.h"
#include "misc.h"
#include "wgt2allg_nofunc.h"
#include "acroom_func.h"
//

//=============================================================================
// Engine-specific implementation split out of acroom.cpp
//=============================================================================

bool load_room_is_version_bad(roomstruct *rstruc)
{
  // CHECKME: agsnative variant has '||' operator here, is this a typo?
  return ((rstruc->wasversion < 17) | (rstruc->wasversion > ROOM_FILE_VERSION));
}
