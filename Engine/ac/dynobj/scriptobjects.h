//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// A collection of structs wrapping a reference to particular game object
// types. These are allocated in the script managed pool and exported
// to the script.
//
// For historical reasons must be at least 8-bytes large (actual contents
// are not restricted now).
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H
#define __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H

struct ScriptSimpleRef
{
    int id = -1;
    int reserved = 0;
};

struct ScriptAudioChannel : public ScriptSimpleRef {};
// NOTE: ScriptCharacter is a dummy placeholder
struct ScriptCharacter : public ScriptSimpleRef {};
struct ScriptDialog : public ScriptSimpleRef {};
struct ScriptGUI : public ScriptSimpleRef {};
struct ScriptHotspot : public ScriptSimpleRef {};
struct ScriptInvItem : public ScriptSimpleRef {};
struct ScriptObject : public ScriptSimpleRef {};
struct ScriptRegion : public ScriptSimpleRef {};
struct ScriptWalkableArea : public ScriptSimpleRef {};
struct ScriptWalkbehind : public ScriptSimpleRef {};

#endif // __AGS_EE_DYNOBJ__SCRIPTOBJECTS_H
