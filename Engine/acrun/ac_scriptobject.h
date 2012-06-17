#ifndef __AC_SCRIPTOBJECT_H
#define __AC_SCRIPTOBJECT_H

#include "acrun/ac_roomobject.h"
#include "acgui/ac_guimain.h"

struct ScriptObject {
    int id;
    RoomObject *obj;
};

struct ScriptGUI {
    int id;
    GUIMain *gui;
};

struct ScriptHotspot {
    int id;
    int reserved;
};

struct ScriptRegion {
    int id;
    int reserved;
};

struct ScriptDialog {
    int id;
    int reserved;
};

struct ScriptInvItem {
    int id;
    int reserved;
};

#endif // __AC_SCRIPTOBJECT_H