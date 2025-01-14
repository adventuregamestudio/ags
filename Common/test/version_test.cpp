//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gtest/gtest.h"
#include "util/version.h"

using namespace AGS::Common;

TEST(Version, Format) {
    // Old version format
    Version test_ver1 = Version("2.06.101");
    Version test_ver2 = Version("2.56.654");
    Version test_ver3 = Version("2.7.722");
    Version test_ver4 = Version("2.72.872NMP");
    Version test_ver5 = Version("3.2.0");
    Version test_ver6 = Version("3.21.1115");
    Version test_ver7 = Version("3.21.1115NMP");
    // New version format
    Version test_ver8 = Version("3.3.0.1130");
    Version test_ver9 = Version("3.3.0.1130 BETA");

    ASSERT_TRUE(test_ver1.Major == 2);
    ASSERT_TRUE(test_ver1.Minor == 0);
    ASSERT_TRUE(test_ver1.Release == 6);
    ASSERT_TRUE(test_ver1.Revision == 101);
    ASSERT_TRUE(strcmp(test_ver1.LongString.GetCStr(), "2.0.6.101") == 0);
    ASSERT_TRUE(strcmp(test_ver1.BackwardCompatibleString.GetCStr(), "2.06.101") == 0);
    ASSERT_TRUE(strcmp(test_ver1.ShortString.GetCStr(), "2.0") == 0);

    ASSERT_TRUE(test_ver2.Major == 2);
    ASSERT_TRUE(test_ver2.Minor == 5);
    ASSERT_TRUE(test_ver2.Release == 6);
    ASSERT_TRUE(test_ver2.Revision == 654);
    ASSERT_TRUE(strcmp(test_ver2.LongString.GetCStr(), "2.5.6.654") == 0);
    ASSERT_TRUE(strcmp(test_ver2.BackwardCompatibleString.GetCStr(), "2.56.654") == 0);
    ASSERT_TRUE(strcmp(test_ver2.ShortString.GetCStr(), "2.5") == 0);

    ASSERT_TRUE(test_ver3.Major == 2);
    ASSERT_TRUE(test_ver3.Minor == 7);
    ASSERT_TRUE(test_ver3.Release == 0);
    ASSERT_TRUE(test_ver3.Revision == 722);
    ASSERT_TRUE(strcmp(test_ver3.LongString.GetCStr(), "2.7.0.722") == 0);
    ASSERT_TRUE(strcmp(test_ver3.BackwardCompatibleString.GetCStr(), "2.70.722") == 0);
    ASSERT_TRUE(strcmp(test_ver3.ShortString.GetCStr(), "2.7") == 0);

    ASSERT_TRUE(test_ver4.Major == 2);
    ASSERT_TRUE(test_ver4.Minor == 7);
    ASSERT_TRUE(test_ver4.Release == 2);
    ASSERT_TRUE(test_ver4.Revision == 872);
    ASSERT_TRUE(strcmp(test_ver4.LongString.GetCStr(), "2.7.2.872 NMP") == 0);
    ASSERT_TRUE(strcmp(test_ver4.BackwardCompatibleString.GetCStr(), "2.72.872NMP") == 0);
    ASSERT_TRUE(strcmp(test_ver4.ShortString.GetCStr(), "2.7") == 0);

    ASSERT_TRUE(test_ver5.Major == 3);
    ASSERT_TRUE(test_ver5.Minor == 2);
    ASSERT_TRUE(test_ver5.Release == 0);
    ASSERT_TRUE(test_ver5.Revision == 0);
    ASSERT_TRUE(strcmp(test_ver5.LongString.GetCStr(), "3.2.0.0") == 0);
    ASSERT_TRUE(strcmp(test_ver5.BackwardCompatibleString.GetCStr(), "3.20.0") == 0);
    ASSERT_TRUE(strcmp(test_ver5.ShortString.GetCStr(), "3.2") == 0);

    ASSERT_TRUE(test_ver6.Major == 3);
    ASSERT_TRUE(test_ver6.Minor == 2);
    ASSERT_TRUE(test_ver6.Release == 1);
    ASSERT_TRUE(test_ver6.Revision == 1115);
    ASSERT_TRUE(strcmp(test_ver6.LongString.GetCStr(), "3.2.1.1115") == 0);
    ASSERT_TRUE(strcmp(test_ver6.BackwardCompatibleString.GetCStr(), "3.21.1115") == 0);
    ASSERT_TRUE(strcmp(test_ver6.ShortString.GetCStr(), "3.2") == 0);

    ASSERT_TRUE(test_ver7.Major == 3);
    ASSERT_TRUE(test_ver7.Minor == 2);
    ASSERT_TRUE(test_ver7.Release == 1);
    ASSERT_TRUE(test_ver7.Revision == 1115);
    ASSERT_TRUE(strcmp(test_ver7.LongString.GetCStr(), "3.2.1.1115 NMP") == 0);
    ASSERT_TRUE(strcmp(test_ver7.BackwardCompatibleString.GetCStr(), "3.21.1115NMP") == 0);
    ASSERT_TRUE(strcmp(test_ver7.ShortString.GetCStr(), "3.2") == 0);

    ASSERT_TRUE(test_ver8.Major == 3);
    ASSERT_TRUE(test_ver8.Minor == 3);
    ASSERT_TRUE(test_ver8.Release == 0);
    ASSERT_TRUE(test_ver8.Revision == 1130);
    ASSERT_TRUE(strcmp(test_ver8.LongString.GetCStr(), "3.3.0.1130") == 0);
    ASSERT_TRUE(strcmp(test_ver8.BackwardCompatibleString.GetCStr(), "3.30.1130") == 0);
    ASSERT_TRUE(strcmp(test_ver8.ShortString.GetCStr(), "3.3") == 0);

    ASSERT_TRUE(test_ver9.Major == 3);
    ASSERT_TRUE(test_ver9.Minor == 3);
    ASSERT_TRUE(test_ver9.Release == 0);
    ASSERT_TRUE(test_ver9.Revision == 1130);
    ASSERT_TRUE(strcmp(test_ver9.LongString.GetCStr(), "3.3.0.1130 BETA") == 0);
    ASSERT_TRUE(strcmp(test_ver9.BackwardCompatibleString.GetCStr(), "3.30.1130BETA") == 0);
    ASSERT_TRUE(strcmp(test_ver9.ShortString.GetCStr(), "3.3") == 0);
}
