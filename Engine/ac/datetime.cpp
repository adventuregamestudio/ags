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
#include <time.h>
#include "ac/datetime.h"
#include "ac/dynobj/dynobj_manager.h"
#include "platform/base/agsplatformdriver.h"

ScriptDateTime* DateTime_Now_Core() {
    ScriptDateTime *sdt = new ScriptDateTime();

    platform->GetSystemTime(sdt);

    return sdt;
}

ScriptDateTime* DateTime_Now() {
    ScriptDateTime *sdt = DateTime_Now_Core();
    ccRegisterManagedObject(sdt, sdt);
    return sdt;
}

int DateTime_GetYear(ScriptDateTime *sdt) {
    return sdt->year;
}

int DateTime_GetMonth(ScriptDateTime *sdt) {
    return sdt->month;
}

int DateTime_GetDayOfMonth(ScriptDateTime *sdt) {
    return sdt->day;
}

int DateTime_GetHour(ScriptDateTime *sdt) {
    return sdt->hour;
}

int DateTime_GetMinute(ScriptDateTime *sdt) {
    return sdt->minute;
}

int DateTime_GetSecond(ScriptDateTime *sdt) {
    return sdt->second;
}

int DateTime_GetRawTime(ScriptDateTime *sdt) {
    return sdt->rawUnixTime;
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
