
#include <stdio.h>
#include "wgt2allg.h"
#include "acmain/ac_maindefines.h"
#include "acrun/ac_datetime.h"
#include "acrun/ac_platformdriver.h"
#include "ac/ac_common.h"

// ** SCRIPT DATETIME OBJECT





ScriptDateTime* DateTime_Now_Core() {
    ScriptDateTime *sdt = new ScriptDateTime();
    sdt->rawUnixTime = time(NULL);

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

int sc_GetTime(int whatti) {
    ScriptDateTime *sdt = DateTime_Now_Core();
    int returnVal;

    if (whatti == 1) returnVal = sdt->hour;
    else if (whatti == 2) returnVal = sdt->minute;
    else if (whatti == 3) returnVal = sdt->second;
    else if (whatti == 4) returnVal = sdt->day;
    else if (whatti == 5) returnVal = sdt->month;
    else if (whatti == 6) returnVal = sdt->year;
    else quit("!GetTime: invalid parameter passed");

    delete sdt;

    return returnVal;
}

int GetRawTime () {
    return time(NULL);
}
