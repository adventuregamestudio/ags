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
#include <ctime>
#include "ac/dynobj/dynobj_manager.h"
#include "util/stream.h"

using namespace AGS::Common;

int ScriptDateTime::Dispose(void* /*address*/, bool /*force*/)
{
    // always dispose a DateTime
    delete this;
    return 1;
}

const char *ScriptDateTime::GetType()
{
    return "DateTime";
}

ScriptDateTime::ScriptDateTime(const time_t &time)
{
    SetTime(SystemClock::from_time_t(time));
}

ScriptDateTime::ScriptDateTime(const ClockTimePoint &time)
{
    SetTime(time);
}

void ScriptDateTime::SetTime(const ClockTimePoint &time)
{
    // NOTE: subject to year 2038 problem due to shoving seconds since epoch into an int32
    _rawtime = static_cast<int32_t>(
        std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count());

    std::time_t ttime = SystemClock::to_time_t(time);
    std::tm *newtime = std::localtime(&ttime);
    _hour = newtime->tm_hour;
    _minute = newtime->tm_min;
    _second = newtime->tm_sec;
    _day = newtime->tm_mday;
    _month = newtime->tm_mon + 1;
    _year = newtime->tm_year + 1900;
}

size_t ScriptDateTime::CalcSerializeSize(const void* /*address*/)
{
    return sizeof(int32_t) * 7;
}

void ScriptDateTime::Serialize(const void* /*address*/, Stream *out)
{
    out->WriteInt32(_year);
    out->WriteInt32(_month);
    out->WriteInt32(_day);
    out->WriteInt32(_hour);
    out->WriteInt32(_minute);
    out->WriteInt32(_second);
    out->WriteInt32(_rawtime);
}

void ScriptDateTime::Unserialize(int index, Stream *in, size_t /*data_sz*/)
{
    _year = in->ReadInt32();
    _month = in->ReadInt32();
    _day = in->ReadInt32();
    _hour = in->ReadInt32();
    _minute = in->ReadInt32();
    _second = in->ReadInt32();
    _rawtime = in->ReadInt32();
    ccRegisterUnserializedObject(index, this, this);
}
