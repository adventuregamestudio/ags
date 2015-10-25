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

#ifdef _DEBUG

#include "test/test_all.h"
#include "test/test_string.h"
#include "test/test_file.h"

void Test_DoAllTests()
{
    Test_ScriptSprintf();
    Test_String();
    Test_Version();
    Test_File();
    Test_IniFile();

    Test_Gfx();
}

#endif // _DEBUG
