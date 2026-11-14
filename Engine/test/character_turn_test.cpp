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
//
// Character turning (start_character_turning): the direction and the length of
// a turn, including the tie-break of a 180 degrees turn by the turn order
// priority game option.
//
//=============================================================================
#include <algorithm>
#include <vector>
#include "gtest/gtest.h"
#include "ac/character.h"
#include "ac/gamesetupstruct.h"
#include "ac/runtime_defines.h"
#include "ac/view.h"

extern GameSetupStruct game;
extern std::vector<ViewStruct> views;

// Fixture: one character with an 8-loop view of one frame per loop
class CharacterTurning : public ::testing::Test
{
protected:
    void SetUp() override
    {
        views.resize(1);
        views[0].Initialize(MAX_FACE_DIRECTIONS);
        for (int i = 0; i < MAX_FACE_DIRECTIONS; ++i)
            views[0].loops[i].Initialize(1);
        game.chars.resize(1);
        game.numcharacters = 1;
        game.chars[0].view = 0;
        saved_priority = game.options[OPT_TURNORDERPRIORITY];
    }

    void TearDown() override
    {
        game.options[OPT_TURNORDERPRIORITY] = saved_priority;
        game.chars.clear();
        game.numcharacters = 0;
        views.clear();
    }

    // Starts turning from the loop at index from_idx of turnlooporder to the one at to_idx;
    // returns the indices of turnlooporder that the character passes between the two,
    // empty if it does not turn
    std::vector<int> Turn(int from_idx, int to_idx)
    {
        game.chars[0].loop = turnlooporder[from_idx];
        game.chars[0].walking = 0;
        start_character_turning(&game.chars[0], turnlooporder[to_idx], true);
        std::vector<int> passed;
        if (!game.chars[0].is_turning())
            return passed;
        // walking is the movelist id (0 here) plus TURNING_AROUND for each loop left behind,
        // the start included, plus TURNING_BACKWARDS for an anti-clockwise turn
        const bool backwards = game.chars[0].walking >= TURNING_BACKWARDS;
        const int turns = (game.chars[0].walking - (backwards ? TURNING_BACKWARDS : 0)) / TURNING_AROUND;
        const int step = backwards ? -1 : 1;
        // the last loop left behind leads to the target
        for (int i = 1; i < turns; ++i)
            passed.push_back(((from_idx + step * i) % MAX_FACE_DIRECTIONS + MAX_FACE_DIRECTIONS) % MAX_FACE_DIRECTIONS);
        return passed;
    }

    int saved_priority = 0;
};

TEST_F(CharacterTurning, ShorterWayIsTaken)
{
    game.options[OPT_TURNORDERPRIORITY] = kScTurnOrder_Clockwise;
    // two steps clockwise: one loop between
    EXPECT_EQ(std::vector<int>({1}), Turn(0, 2));
    // three steps the other way: anti-clockwise through two loops
    EXPECT_EQ(std::vector<int>({1, 0}), Turn(2, 7));
}

TEST_F(CharacterTurning, FaceDownPassesThroughDownFromEveryStart)
{
    game.options[OPT_TURNORDERPRIORITY] = kScTurnOrder_FaceDown;
    const int down_idx = 0;
    const int up_idx = MAX_FACE_DIRECTIONS / 2;
    for (int from_idx = 0; from_idx < MAX_FACE_DIRECTIONS; ++from_idx)
    {
        const int to_idx = (from_idx + MAX_FACE_DIRECTIONS / 2) % MAX_FACE_DIRECTIONS;
        const std::vector<int> passed = Turn(from_idx, to_idx);
        ASSERT_EQ(MAX_FACE_DIRECTIONS / 2 - 1, static_cast<int>(passed.size())) << "from " << from_idx;
        const bool through_down = std::find(passed.begin(), passed.end(), down_idx) != passed.end();
        if (from_idx == down_idx || from_idx == up_idx)
        {
            // Down is an end of this turn, not a loop passed on the way
            EXPECT_FALSE(through_down) << "from " << from_idx;
        }
        else
        {
            EXPECT_TRUE(through_down) << "from " << from_idx;
        }
    }
}
