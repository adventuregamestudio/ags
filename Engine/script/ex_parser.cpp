
//=============================================================================
//
// Exporting Parser script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_parser_script_functions()
{
	scAdd_External_Symbol("Parser::FindWordID^1",(void *)Parser_FindWordID);
	scAdd_External_Symbol("Parser::ParseText^1",(void *)ParseText);
	scAdd_External_Symbol("Parser::SaidUnknownWord^0",(void *)Parser_SaidUnknownWord);
	scAdd_External_Symbol("Parser::Said^1",(void *)Said);
}
