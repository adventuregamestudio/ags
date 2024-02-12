//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gtest/gtest.h"
#include "util/path.h"

using namespace AGS::Common;

TEST(Path, IsSameOrSubDir) {
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "dir1/") == true);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "dir1/dir2/dir3/") == true);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "dir1/../") == true);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "dir1/dir2/../../") == true);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "dir1/../dir2/../dir3/") == true);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "..dir/") == true);

    ASSERT_TRUE(Path::IsSameOrSubDir(".", "../") == false);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "../") == false);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "/dir1/") == false);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "dir1/../../") == false);
    ASSERT_TRUE(Path::IsSameOrSubDir(".", "dir1/../dir2/../../dir3/") == false);
}
