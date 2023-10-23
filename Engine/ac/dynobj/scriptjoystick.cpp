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

#include "ac/dynobj/scriptjoystick.h"
#include "ac/joystick.h"
#include "util/stream.h"
#include "dynobj_manager.h"

using namespace AGS::Common;

ScriptJoystick::ScriptJoystick(int id) : _id(id) {}

const char *ScriptJoystick::GetType() {
    return "Joystick";
}

int ScriptJoystick::Dispose(void *address, bool force) {
    delete this;
    return 1;
}

size_t ScriptJoystick::CalcSerializeSize(const void* /*address*/){
    return 0;
}

void ScriptJoystick::Serialize(const void *address, AGS::Common::Stream *out) {

}

void ScriptJoystick::Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) {
    this->Invalidate();
    ccRegisterUnserializedObject(index, this, this);
}