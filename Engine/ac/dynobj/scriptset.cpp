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
#include "ac/dynobj/scriptset.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

int ScriptSetBase::Dispose(void* /*address*/, bool /*force*/)
{
    Clear();
    delete this;
    return 1;
}

const char *ScriptSetBase::GetType()
{
    return "StringSet";
}

size_t ScriptSetBase::CalcSerializeSize(const void* /*address*/)
{
    return CalcContainerSize();
}

void ScriptSetBase::Serialize(const void* /*address*/, Stream *out)
{
    out->WriteInt32(IsSorted());
    out->WriteInt32(IsCaseSensitive());
    SerializeContainer(out);
}

void ScriptSetBase::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    // NOTE: we expect sorted/case flags are read by external reader;
    // this is awkward, but I did not find better design solution atm
    UnserializeContainer(in);
    ccRegisterUnserializedObject(index, this, this);
}
