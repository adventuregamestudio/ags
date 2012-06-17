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

// The text script's "mouse" struct
struct ScriptMouse {
    int x,y;
};

// The text script's "system" struct
struct ScriptSystem {
    int width,height;
    int coldepth;
    int os;
    int windowed;
    int vsync;
    int viewport_width, viewport_height;
    char aci_version[10];
    int reserved[5];  // so that future scripts don't overwrite data
};

#endif // __AC_SCRIPTOBJECT_H