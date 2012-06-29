
#include "ac/datetime.h"
#include "wgt2allg.h"
#include "platform/agsplatformdriver.h"

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
