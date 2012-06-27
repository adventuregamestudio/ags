
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_AC__DATETIME_H
#define __AGS_EE_AC__DATETIME_H

#include "ac/dynobj/scriptdatetime.h"

ScriptDateTime* DateTime_Now_Core();
ScriptDateTime* DateTime_Now();
int             DateTime_GetYear(ScriptDateTime *sdt);
int             DateTime_GetMonth(ScriptDateTime *sdt);
int             DateTime_GetDayOfMonth(ScriptDateTime *sdt);
int             DateTime_GetHour(ScriptDateTime *sdt);
int             DateTime_GetMinute(ScriptDateTime *sdt);
int             DateTime_GetSecond(ScriptDateTime *sdt);
int             DateTime_GetRawTime(ScriptDateTime *sdt);

#endif // __AGS_EE_AC__DATETIME_H
