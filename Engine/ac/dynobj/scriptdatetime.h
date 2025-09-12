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
//
// ScriptDateTime is a script object that contains date/time definition.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTDATETIME_H
#define __AGS_EE_DYNOBJ__SCRIPTDATETIME_H

#include <chrono>
#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptDateTime final : AGSCCDynamicObject
{
public:
    typedef std::chrono::system_clock SystemClock;
    typedef std::chrono::time_point<SystemClock> ClockTimePoint;

    // Constructs DateTime initialized with zero time
    ScriptDateTime() = default;
    // Constructs DateTime initialized using chrono::time_point
    ScriptDateTime(const ClockTimePoint &time);

    // Constructs DateTime initialized using C time_t
    static ScriptDateTime *FromStdTime(const time_t &time);
    // Constructs DateTime initialized with raw time (unix time)
    static ScriptDateTime *FromRawTime(int raw_time);
    // Constructs DateTime initialized with all the date/time components
    static ScriptDateTime *FromFullDate(int year, int month, int day, int hour, int minute, int second);

    int Dispose(void *address, bool force) override;
    const char *GetType() override;
    void Unserialize(int index, AGS::Common::Stream *in, size_t data_sz) override;

    inline int RawTime() const { return _rawtime; }
    inline int Year() const { return _year; }
    inline int Month() const { return _month; }
    inline int Day() const { return _day; }
    inline int Hour() const { return _hour; }
    inline int Minute() const { return _minute; }
    inline int Second() const { return _second; }

private:
    int _year = 0, _month = 0, _day = 0;
    int _hour = 0, _minute = 0, _second = 0;
    int _rawtime = -1;

    void SetTime(const ClockTimePoint &time);

    // Calculate and return required space for serialization, in bytes
    size_t CalcSerializeSize(const void *address) override;
    // Write object data into the provided stream
    void Serialize(const void *address, AGS::Common::Stream *out) override;
};

#endif // __AGS_EE_DYNOBJ__SCRIPTDATETIME_H
