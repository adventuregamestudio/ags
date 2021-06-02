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
