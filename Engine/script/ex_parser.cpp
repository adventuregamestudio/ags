
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
	ccAddExternalStaticFunction("Parser::FindWordID^1",(void *)Parser_FindWordID);
	ccAddExternalStaticFunction("Parser::ParseText^1",(void *)ParseText);
	ccAddExternalStaticFunction("Parser::SaidUnknownWord^0",(void *)Parser_SaidUnknownWord);
	ccAddExternalStaticFunction("Parser::Said^1",(void *)Said);
}
