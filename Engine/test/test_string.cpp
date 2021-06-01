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
#ifdef AGS_RUN_TESTS

#include <string.h>
#include <vector>
#include "util/path.h"
#include "util/string.h"
#include "debug/assert.h"

using namespace AGS::Common;

void Test_Path()
{
    assert(Path::IsSameOrSubDir(".", "dir1/") == true);
    assert(Path::IsSameOrSubDir(".", "dir1/dir2/dir3/") == true);
    assert(Path::IsSameOrSubDir(".", "dir1/../") == true);
    assert(Path::IsSameOrSubDir(".", "dir1/dir2/../../") == true);
    assert(Path::IsSameOrSubDir(".", "dir1/../dir2/../dir3/") == true);
    assert(Path::IsSameOrSubDir(".", "..dir/") == true);

    assert(Path::IsSameOrSubDir(".", "../") == false);
    assert(Path::IsSameOrSubDir(".", "../") == false);
    assert(Path::IsSameOrSubDir(".", "/dir1/") == false);
    assert(Path::IsSameOrSubDir(".", "dir1/../../") == false);
    assert(Path::IsSameOrSubDir(".", "dir1/../dir2/../../dir3/") == false);
}

void Test_String()
{
}

#endif // AGS_RUN_TESTS
