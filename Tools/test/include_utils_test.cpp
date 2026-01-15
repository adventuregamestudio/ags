//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include <vector>
#include <algorithm>
#include "gtest/gtest.h"
#include "util/string.h"
#include "data/include_utils.h"

using namespace AGS;
using namespace AGS::Common;

bool contains(const std::vector<String>& vec, const String& value)
{
    return std::find(vec.begin(), vec.end(), value) != vec.end();
}

TEST(IncludeUtils, SimpleMatch)
{
    const std::vector<String> patterns = {
        "*.txt",
        "!exclude.txt"
    };

    const std::vector<String> files = {
        "file.txt",
        "exclude.txt"
    };

    std::vector<String> matches{};

    DataUtil::MatchPatternPaths(files, matches, patterns);

    ASSERT_EQ(matches.size(), 1);
}

TEST(IncludeUtils, CaseInsensitiveMatch)
{
    const std::vector<String> files = {
        "file1.txt",
        "file2.TXT",
        "file3.TxT"
    };

    const std::vector<String> patterns = {
        "*.TXT"
    };

    std::vector<String> matches{};

    DataUtil::MatchPatternPaths(files, matches, patterns);

    ASSERT_EQ(matches.size(), 3);
}

TEST(IncludeUtils, InclusionAndExclusion)
{
    const std::vector<String> files = {
        "a.txt",
        "b.txt",
        "include_me/c.txt",
        "include_me/d.txt",
        "include_me/exclude_me_anyway.dat",
        "exclude_me/file1.dat",
        "exclude_me/file2.dat",
        "exclude_me/e.txt",
        "exclude_me/f.txt",
        "exclude_me/include_me_anyway.dat"
    };

    const std::vector<String> patterns = {
        "*.txt",
        "!exclude_me_anyway.dat",
        "!exclude_me/",
        "include_me_anyway.dat"
    };

    std::vector<String> matches{};

    DataUtil::MatchPatternPaths(files, matches, patterns);

    ASSERT_EQ(matches.size(), 5);
    ASSERT_TRUE(contains(matches, "a.txt"));
    ASSERT_TRUE(contains(matches, "b.txt"));
    ASSERT_TRUE(contains(matches, "include_me/c.txt"));
    ASSERT_TRUE(contains(matches, "include_me/d.txt"));
    ASSERT_TRUE(contains(matches, "exclude_me/include_me_anyway.dat"));
}

TEST(IncludeUtils, PatternOrderMatters)
{
    const std::vector<String> files = {
        "file.txt",
        "exclude.txt"
    };

    const std::vector<String> patterns = {
        "*.txt",
        "!exclude.txt",
        "*.txt"
    };

    std::vector<String> matches{};

    DataUtil::MatchPatternPaths(files, matches, patterns);

    ASSERT_TRUE(contains(matches, "file.txt"));
    ASSERT_TRUE(contains(matches,"exclude.txt"));
}

TEST(IncludeUtils, MatchAnySectionOfThePath)
{
    const std::vector<String> files = {
        "name",
        "name/file1.dat",
        "name/file2.dat",
        "name/file3.dat",
        "name.dir/file4.dat",
        "dir/name",
        "dir/name/file5.dat",
        "dir/name.dat"
    };

    const std::vector<String> patterns = {
        "name"
    };

    std::vector<String> matches{};

    DataUtil::MatchPatternPaths(files, matches, patterns);

    ASSERT_EQ(matches.size(), 6);
    ASSERT_TRUE(contains(matches, "name"));
    ASSERT_TRUE(contains(matches, "name/file1.dat"));
    ASSERT_TRUE(contains(matches, "name/file2.dat"));
    ASSERT_TRUE(contains(matches, "name/file3.dat"));
    ASSERT_TRUE(contains(matches, "dir/name"));
    ASSERT_TRUE(contains(matches, "dir/name/file5.dat"));
}

TEST(IncludeUtils, SpacesInPathsAndPatterns)
{
    const std::vector<String> files = {
        "file with space.txt",
        "another file.txt",
        "some dir/file.txt",
        "some dir/file with space.txt",
        "excluded file.txt",
        "some other/file.txt"
    };

    const std::vector<String> patterns = {
        "* with space.txt",
        "some dir/*",
        "*.txt",
        "!excluded file.txt",
        "!some dir/file *"
    };

    std::vector<String> matches{};

    DataUtil::MatchPatternPaths(files, matches, patterns);

    ASSERT_EQ(matches.size(), 4);
    ASSERT_TRUE(contains(matches, "file with space.txt"));
    ASSERT_TRUE(contains(matches, "another file.txt"));
    ASSERT_TRUE(contains(matches, "some dir/file.txt"));
    ASSERT_TRUE(contains(matches, "some other/file.txt"));
    ASSERT_FALSE(contains(matches, "excluded file.txt"));
    ASSERT_FALSE(contains(matches, "some dir/file with space.txt"));
}

TEST(IncludeUtils, ExampleTemplate)
{
    const std::vector<String> files = {
        "acsprset.spr",
        "AGSFNT0.WFN",
        "AGSFNT1.WFN",
        "AGSFNT2.WFN",
        "AudioCache",
        "Compiled",
        "Compiled/Data",
        "Game.agf",
        "Game.agf.bak",
        "Game.agf.user",
        "GlobalScript.asc",
        "GlobalScript.ash",
        "KeyboardMovement.asc",
        "KeyboardMovement.ash",
        "KeyboardMovement.txt",
        "room1.asc",
        "room1.crm",
        "Speech",
        "sprindex.dat",
        "Sprites",
        "Sprites/bluecup0.png",
        "Sprites/Defaults",
        "Sprites/Defaults/Cursors",
        "Sprites/Defaults/Cursors/cursor_interact.png",
        "Sprites/Defaults/ico_bluecup.png",
        "Sprites/Defaults/ico_key.png",
        "Sprites/Defaults/ico_x.png",
        "Sprites/Defaults/UI",
        "Sprites/Defaults/UI/background_inventory.png",
        "Sprites/Defaults/Verbs",
        "Sprites/Defaults/Verbs/btn_arrowdown_normal.png",
        "Sprites/Defaults/Verbs/btn_arrowdown_over.png",
        "template.files",
        "template.ico",
        "template.txt",
        "_Debug",
        "_Debug/acsetup.cfg",
        "_Debug/SDL2.dll",
        "_Debug/Sierra-style.exe"
    };

    const std::vector<String> patterns = {
        "*.asc",
        "*.ash",
        "*.crm",
        "*.ttf",
        "*.wfn",
        "*.txt",
        "Game.agf",
        "template.files",
        "template.ico",
        "Speech/",
        "Sprites/",
        "!_Debug/",
        "!*.bak",
        "!*.user"
    };

    std::vector<String> matches{};

    DataUtil::MatchPatternPaths(files, matches, patterns);

    ASSERT_EQ(matches.size(), 26);
    ASSERT_TRUE(contains(matches, "AGSFNT0.WFN"));
    ASSERT_TRUE(contains(matches, "AGSFNT1.WFN"));
    ASSERT_TRUE(contains(matches, "AGSFNT2.WFN"));
    ASSERT_TRUE(contains(matches, "Game.agf"));
    ASSERT_TRUE(contains(matches, "GlobalScript.asc"));
    ASSERT_TRUE(contains(matches, "GlobalScript.ash"));
    ASSERT_TRUE(contains(matches, "KeyboardMovement.asc"));
    ASSERT_TRUE(contains(matches, "KeyboardMovement.ash"));
    ASSERT_TRUE(contains(matches, "KeyboardMovement.txt"));
    ASSERT_TRUE(contains(matches, "room1.asc"));
    ASSERT_TRUE(contains(matches, "room1.crm"));
    ASSERT_TRUE(contains(matches, "Sprites/bluecup0.png"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/Cursors"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/Cursors/cursor_interact.png"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/ico_bluecup.png"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/ico_key.png"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/ico_x.png"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/UI"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/UI/background_inventory.png"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/Verbs"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/Verbs/btn_arrowdown_normal.png"));
    ASSERT_TRUE(contains(matches, "Sprites/Defaults/Verbs/btn_arrowdown_over.png"));
    ASSERT_TRUE(contains(matches, "template.files"));
    ASSERT_TRUE(contains(matches, "template.ico"));
    ASSERT_TRUE(contains(matches, "template.txt"));
}
