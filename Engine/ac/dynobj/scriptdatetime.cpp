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
#include "ac/dynobj/scriptdatetime.h"
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

int ScriptDateTime::Dispose(void* /*address*/, bool /*force*/) {
    // always dispose a DateTime
    delete this;
    return 1;
}

const char *ScriptDateTime::GetType() {
    return "DateTime";
}

size_t ScriptDateTime::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t) * 7;
}

void ScriptDateTime::Serialize(const void* /*address*/, Stream *out) {
    out->WriteInt32(year);
    out->WriteInt32(month);
    out->WriteInt32(day);
    out->WriteInt32(hour);
    out->WriteInt32(minute);
    out->WriteInt32(second);
    out->WriteInt32(rawUnixTime);
}

void ScriptDateTime::Unserialize(int index, Stream *in, size_t /*data_sz*/) {
    year = in->ReadInt32();
    month = in->ReadInt32();
    day = in->ReadInt32();
    hour = in->ReadInt32();
    minute = in->ReadInt32();
    second = in->ReadInt32();
    rawUnixTime = in->ReadInt32();
    ccRegisterUnserializedObject(index, this, this);
}

ScriptDateTime::ScriptDateTime() {
    year = month = day = 0;
    hour = minute = second = 0;
    rawUnixTime = 0;
}
