
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTOBJECT_H
#define __AGS_EE_DYNOBJ__SCRIPTOBJECT_H

#include "ac/roomobject.h"

// 64 bit: Struct size must be 8 byte for scripts to work
struct ScriptObject {
  int id;
  //RoomObject *obj;
  int __padding;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTOBJECT_H
