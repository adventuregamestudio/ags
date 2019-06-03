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

#include "core/platform.h"
#if AGS_PLATFORM_DEBUG

void Test_DoAllTests();
// Math tests
void Test_Math();
// File tests
void Test_File();
void Test_IniFile();
// Graphics tests
void Test_Gfx();
// Memory / bit-byte operations
void Test_Memory();
// String tests
void Test_ScriptSprintf();
void Test_String();
void Test_Path();
void Test_Version();

#endif // AGS_PLATFORM_DEBUG
