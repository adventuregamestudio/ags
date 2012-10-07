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
	scAdd_External_Symbol("File::Delete^1",(void *)File_Delete);
	scAdd_External_Symbol("File::Exists^1",(void *)File_Exists);
	scAdd_External_Symbol("File::Open^2",(void *)sc_OpenFile);
	scAdd_External_Symbol("File::Close^0", (void *)File_Close);
	scAdd_External_Symbol("File::ReadInt^0", (void *)File_ReadInt);
	scAdd_External_Symbol("File::ReadRawChar^0", (void *)File_ReadRawChar);
	scAdd_External_Symbol("File::ReadRawInt^0", (void *)File_ReadRawInt);
	scAdd_External_Symbol("File::ReadRawLine^1", (void *)File_ReadRawLine);
	scAdd_External_Symbol("File::ReadRawLineBack^0", (void *)File_ReadRawLineBack);
	scAdd_External_Symbol("File::ReadString^1", (void *)File_ReadString);
	scAdd_External_Symbol("File::ReadStringBack^0", (void *)File_ReadStringBack);
	scAdd_External_Symbol("File::WriteInt^1", (void *)File_WriteInt);
	scAdd_External_Symbol("File::WriteRawChar^1", (void *)File_WriteRawChar);
	scAdd_External_Symbol("File::WriteRawLine^1", (void *)File_WriteRawLine);
	scAdd_External_Symbol("File::WriteString^1", (void *)File_WriteString);
	scAdd_External_Symbol("File::get_EOF", (void *)File_GetEOF);
	scAdd_External_Symbol("File::get_Error", (void *)File_GetError);
}
