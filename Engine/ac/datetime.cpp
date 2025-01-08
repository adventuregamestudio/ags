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
#include "ac/datetime.h"
#include "ac/dynobj/dynobj_manager.h"
#include "debug/debug_log.h"
#include "platform/base/agsplatformdriver.h"


ScriptDateTime* DateTime_Now() {
    ScriptDateTime *sdt = new ScriptDateTime(ScriptDateTime::SystemClock::now());
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

ScriptDateTime* DateTime_CreateFromDate(int year, int month, int day, int hour, int minute, int second) {
    // NOTE: implementation cannot handle years before 1970,
    // and since DateTime.RawTime is int32, years from 2038 and above cause overflow.
    ScriptDateTime *sdt;
    if (year < 1970 || year >= 2038 || month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59)
    {
        debug_script_warn("DateTime.CreateFromDate: requested date or time is not in a supported range: %dy,%dm,%dd %dh:%dm:%ds",
            year, month, day, hour, minute, second);
        sdt = new ScriptDateTime();
    }
    else
    {
        sdt = new ScriptDateTime(year, month, day, hour, minute, second);
    }
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

ScriptDateTime* DateTime_CreateFromRawTime(int raw_time) {
    ScriptDateTime *sdt;
    if (raw_time < 0)
    {
        debug_script_warn("DateTime.CreateFromRawTime: raw time is not in a supported range: %d", raw_time);
        sdt = new ScriptDateTime();
    }
    else
    {
        sdt = new ScriptDateTime(raw_time);
    }
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

int DateTime_GetYear(ScriptDateTime *sdt) {
    return sdt->Year();
}

int DateTime_GetMonth(ScriptDateTime *sdt) {
    return sdt->Month();
}

int DateTime_GetDayOfMonth(ScriptDateTime *sdt) {
    return sdt->Day();
}

int DateTime_GetHour(ScriptDateTime *sdt) {
    return sdt->Hour();
}

int DateTime_GetMinute(ScriptDateTime *sdt) {
    return sdt->Minute();
}

int DateTime_GetSecond(ScriptDateTime *sdt) {
    return sdt->Second();
}

int DateTime_GetRawTime(ScriptDateTime *sdt) {
    return sdt->RawTime();
}

//=============================================================================
//
// Script API Functions
//
//=============================================================================

#include "debug/out.h"
#include "script/script_api.h"
#include "script/script_runtime.h"

// ScriptDateTime* ()
RuntimeScriptValue Sc_DateTime_Now(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO(ScriptDateTime, DateTime_Now);
}

RuntimeScriptValue Sc_DateTime_CreateFromRawTime(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT(ScriptDateTime, DateTime_CreateFromRawTime);
}

RuntimeScriptValue Sc_DateTime_CreateFromDate(const RuntimeScriptValue *params, int32_t param_count)
{
    API_SCALL_OBJAUTO_PINT6(ScriptDateTime, DateTime_CreateFromDate);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetYear(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetYear);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetMonth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetMonth);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetDayOfMonth(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetDayOfMonth);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetHour(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetHour);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetMinute(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetMinute);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetSecond(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetSecond);
}

// int (ScriptDateTime *sdt)
RuntimeScriptValue Sc_DateTime_GetRawTime(void *self, const RuntimeScriptValue *params, int32_t param_count)
{
    API_OBJCALL_INT(ScriptDateTime, DateTime_GetRawTime);
}

void RegisterDateTimeAPI()
{
    ScFnRegister datetime_api[] = {
        { "DateTime::get_Now",        API_FN_PAIR(DateTime_Now) },
        { "DateTime::CreateFromDate", API_FN_PAIR(DateTime_CreateFromDate) },
        { "DateTime::CreateFromRawTime", API_FN_PAIR(DateTime_CreateFromRawTime) },

        { "DateTime::get_DayOfMonth", API_FN_PAIR(DateTime_GetDayOfMonth) },
        { "DateTime::get_Hour",       API_FN_PAIR(DateTime_GetHour) },
        { "DateTime::get_Minute",     API_FN_PAIR(DateTime_GetMinute) },
        { "DateTime::get_Month",      API_FN_PAIR(DateTime_GetMonth) },
        { "DateTime::get_RawTime",    API_FN_PAIR(DateTime_GetRawTime) },
        { "DateTime::get_Second",     API_FN_PAIR(DateTime_GetSecond) },
        { "DateTime::get_Year",       API_FN_PAIR(DateTime_GetYear) },
    };

    ccAddExternalFunctions(datetime_api);
}
