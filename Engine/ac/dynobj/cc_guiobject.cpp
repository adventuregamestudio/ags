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
#include "ac/dynobj/cc_guiobject.h"
#include "ac/dynobj/scriptgui.h"
#include "ac/dynobj/dynobj_manager.h"
#include "gui/guimain.h"
#include "gui/guiobject.h"
#include "util/stream.h"

using namespace AGS::Common;

// return the type name of the object
const char *CCGUIObject::GetType() {
    return "GUIObject";
}

size_t CCGUIObject::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t) * 2;
}

void CCGUIObject::Serialize(const void *address, Stream *out) {
    const GUIObject *guio = static_cast<const GUIObject*>(address);
    out->WriteInt32(guio->ParentId);
    out->WriteInt32(guio->Id);
}

void CCGUIObject::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    int guinum = in->ReadInt32();
    int objnum = in->ReadInt32();
    ccRegisterUnserializedObject(index, guis[guinum].GetControl(objnum), this);
}
