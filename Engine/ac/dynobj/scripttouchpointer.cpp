//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "ac/dynobj/scripttouchpointer.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

// return the type name of the object
const char *ScriptTouchPointer::GetType()
{
    return "TouchPointer";
}

size_t ScriptTouchPointer::CalcSerializeSize(const void * /*address*/)
{
    return sizeof(int32_t);
}

void ScriptTouchPointer::Serialize(const void *address, Stream *out)
{
    out->WriteInt32(_pointerID);
}

void ScriptTouchPointer::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    _pointerID = in->ReadInt32();
    ccRegisterUnserializedPersistentObject(index, this, this);
}
