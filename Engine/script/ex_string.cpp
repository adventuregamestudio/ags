
//=============================================================================
//
// Exporting String script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include <string.h>
#include "script/symbol_registry.h"

void register_string_script_functions()
{
	ccAddExternalObjectFunction("String::IsNullOrEmpty^1", (void*)String_IsNullOrEmpty);
	ccAddExternalObjectFunction("String::Append^1", (void*)String_Append);
	ccAddExternalObjectFunction("String::AppendChar^1", (void*)String_AppendChar);
	ccAddExternalObjectFunction("String::CompareTo^2", (void*)String_CompareTo);
	ccAddExternalObjectFunction("String::Contains^1", (void*)StrContains);
	ccAddExternalObjectFunction("String::Copy^0", (void*)String_Copy);
	ccAddExternalObjectFunction("String::EndsWith^2", (void*)String_EndsWith);
	ccAddExternalObjectFunction("String::Format^101", (void*)String_Format);
	ccAddExternalObjectFunction("String::IndexOf^1", (void*)StrContains);
	ccAddExternalObjectFunction("String::LowerCase^0", (void*)String_LowerCase);
	ccAddExternalObjectFunction("String::Replace^3", (void*)String_Replace);
	ccAddExternalObjectFunction("String::ReplaceCharAt^2", (void*)String_ReplaceCharAt);
	ccAddExternalObjectFunction("String::StartsWith^2", (void*)String_StartsWith);
	ccAddExternalObjectFunction("String::Substring^2", (void*)String_Substring);
	ccAddExternalObjectFunction("String::Truncate^1", (void*)String_Truncate);
	ccAddExternalObjectFunction("String::UpperCase^0", (void*)String_UpperCase);
	ccAddExternalObjectFunction("String::get_AsFloat", (void*)StringToFloat);
	ccAddExternalObjectFunction("String::get_AsInt", (void*)StringToInt);
	ccAddExternalObjectFunction("String::geti_Chars", (void*)String_GetChars);
	ccAddExternalObjectFunction("String::get_Length", (void*)strlen);
}
