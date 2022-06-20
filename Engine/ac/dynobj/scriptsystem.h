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
//
// ScriptSystem is a readable/writeable struct which has been exposed to
// script in older versions of API (deprecated).
// WARNING: it *MUST* keep its size exact to avoid breaking address offsets
// when running old scripts. In case of emergency you may use its reserved
// fields, but it's not recommended to do, as this struct is not a part of
// the modern API anymore.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H
#define __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H

struct ScriptSystem
{
    int width = 0; // game screen width
    int height = 0; // game screen height
    int coldepth = 0; // game's color depth
    int os = 0; // operating system's code (see eScriptSystemOSID)
    int windowed = 0; // windowed/fullscreen flag
    int vsync = 0; // vertical sync flag
    int viewport_width = 0; // game viewport width (normal or letterboxed)
    int viewport_height = 0; // game viewport height (normal or letterboxed)
    char aci_version[10]{}; // engine version string (informational)
    int reserved[5]{}; // reserved fields
};

#endif // __AGS_EE_DYNOBJ__SCRIPTSYSTEM_H
