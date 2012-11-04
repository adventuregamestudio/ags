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

#include "util/wgt2allg.h"
#include "ac/datetime.h"
#include "platform/base/agsplatformdriver.h"
#include "script/runtimescriptvalue.h"

extern RuntimeScriptValue GlobalReturnValue;

ScriptDateTime* DateTime_Now_Core() {
    ScriptDateTime *sdt = new ScriptDateTime();
    sdt->rawUnixTime = time(NULL);

    platform->GetSystemTime(sdt);

    return sdt;
}

ScriptDateTime* DateTime_Now() {
    ScriptDateTime *sdt = DateTime_Now_Core();
    ccRegisterManagedObject(sdt, sdt);
    GlobalReturnValue.SetDynamicObject(sdt, sdt);
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
