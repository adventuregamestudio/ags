
#include "ac/global_datetime.h"
#include "util/wgt2allg.h"
#include "ac/datetime.h"
#include "ac/common.h"

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
