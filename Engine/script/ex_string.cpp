
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
	scAdd_External_Symbol("String::IsNullOrEmpty^1", (void*)String_IsNullOrEmpty);
	scAdd_External_Symbol("String::Append^1", (void*)String_Append);
	scAdd_External_Symbol("String::AppendChar^1", (void*)String_AppendChar);
	scAdd_External_Symbol("String::CompareTo^2", (void*)String_CompareTo);
	scAdd_External_Symbol("String::Contains^1", (void*)StrContains);
	scAdd_External_Symbol("String::Copy^0", (void*)String_Copy);
	scAdd_External_Symbol("String::EndsWith^2", (void*)String_EndsWith);
	scAdd_External_Symbol("String::Format^101", (void*)String_Format);
	scAdd_External_Symbol("String::IndexOf^1", (void*)StrContains);
	scAdd_External_Symbol("String::LowerCase^0", (void*)String_LowerCase);
	scAdd_External_Symbol("String::Replace^3", (void*)String_Replace);
	scAdd_External_Symbol("String::ReplaceCharAt^2", (void*)String_ReplaceCharAt);
	scAdd_External_Symbol("String::StartsWith^2", (void*)String_StartsWith);
	scAdd_External_Symbol("String::Substring^2", (void*)String_Substring);
	scAdd_External_Symbol("String::Truncate^1", (void*)String_Truncate);
	scAdd_External_Symbol("String::UpperCase^0", (void*)String_UpperCase);
	scAdd_External_Symbol("String::get_AsFloat", (void*)StringToFloat);
	scAdd_External_Symbol("String::get_AsInt", (void*)StringToInt);
	scAdd_External_Symbol("String::geti_Chars", (void*)String_GetChars);
	scAdd_External_Symbol("String::get_Length", (void*)strlen);
}
