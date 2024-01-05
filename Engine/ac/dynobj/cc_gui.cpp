//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/dynobj/cc_gui.h"
#include <vector>
#include "ac/dynobj/scriptgui.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

extern std::vector<ScriptGUI> scrGui;

// return the type name of the object
const char *CCGUI::GetType() {
    return "GUI";
}

size_t CCGUI::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t);
}

void CCGUI::Serialize(const void *address, Stream *out) {
    const ScriptGUI *shh = static_cast<const ScriptGUI*>(address);
    out->WriteInt32(shh->id);
}

void CCGUI::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    int num = in->ReadInt32();
    ccRegisterUnserializedObject(index, &scrGui[num], this);
}