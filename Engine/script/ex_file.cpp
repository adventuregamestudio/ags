
//=============================================================================
//
// Exporting File script functions
//
//=============================================================================

// the ^n after the function name is the number of params
// this is to allow an extra parameter to be added in a later
// version without screwing up the stack in previous versions
// (just export both the ^n and the ^m as seperate funcs)

#include "script/symbol_registry.h"

void register_file_script_functions()
{
	ccAddExternalObjectFunction("File::Delete^1",(void *)File_Delete);
	ccAddExternalObjectFunction("File::Exists^1",(void *)File_Exists);
	ccAddExternalStaticFunction("File::Open^2",(void *)sc_OpenFile);
	ccAddExternalObjectFunction("File::Close^0", (void *)File_Close);
	ccAddExternalObjectFunction("File::ReadInt^0", (void *)File_ReadInt);
	ccAddExternalObjectFunction("File::ReadRawChar^0", (void *)File_ReadRawChar);
	ccAddExternalObjectFunction("File::ReadRawInt^0", (void *)File_ReadRawInt);
	ccAddExternalObjectFunction("File::ReadRawLine^1", (void *)File_ReadRawLine);
	ccAddExternalObjectFunction("File::ReadRawLineBack^0", (void *)File_ReadRawLineBack);
	ccAddExternalObjectFunction("File::ReadString^1", (void *)File_ReadString);
	ccAddExternalObjectFunction("File::ReadStringBack^0", (void *)File_ReadStringBack);
	ccAddExternalObjectFunction("File::WriteInt^1", (void *)File_WriteInt);
	ccAddExternalObjectFunction("File::WriteRawChar^1", (void *)File_WriteRawChar);
	ccAddExternalObjectFunction("File::WriteRawLine^1", (void *)File_WriteRawLine);
	ccAddExternalObjectFunction("File::WriteString^1", (void *)File_WriteString);
	ccAddExternalObjectFunction("File::get_EOF", (void *)File_GetEOF);
	ccAddExternalObjectFunction("File::get_Error", (void *)File_GetError);
}
