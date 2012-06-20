
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H
#define __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H

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

#endif // __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H
