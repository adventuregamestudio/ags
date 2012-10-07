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
//
// Exporting DateTime script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_datetime_script_functions()
{
	scAdd_External_Symbol("DateTime::get_Now", (void*)DateTime_Now);
	scAdd_External_Symbol("DateTime::get_DayOfMonth", (void*)DateTime_GetDayOfMonth);
	scAdd_External_Symbol("DateTime::get_Hour", (void*)DateTime_GetHour);
	scAdd_External_Symbol("DateTime::get_Minute", (void*)DateTime_GetMinute);
	scAdd_External_Symbol("DateTime::get_Month", (void*)DateTime_GetMonth);
	scAdd_External_Symbol("DateTime::get_RawTime", (void*)DateTime_GetRawTime);
	scAdd_External_Symbol("DateTime::get_Second", (void*)DateTime_GetSecond);
	scAdd_External_Symbol("DateTime::get_Year", (void*)DateTime_GetYear);
}
