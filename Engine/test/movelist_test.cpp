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
// Move lists: the route finder's path turned into per-step movement
// (Pathfinding::CalculateMoveList and friends), and MoveList stepping along it
// the way characters and objects walk: speeds per axis and per stage, the
// fraction carried over between stages, speed changes in the middle of a walk,
// and the run flows (once, repeat, there and back, backwards).
//
//=============================================================================
#include <cmath>
#include <vector>
#include "gtest/gtest.h"
#include "ac/movelist.h"
#include "ac/route_finder.h"

using namespace AGS::Common;
using namespace AGS::Engine;

namespace
{

MoveList MakeMoveList(const std::vector<Point> &path, float speed_x, float speed_y,
    const RunPathParams &params = RunPathParams::DefaultOnce())
{
    MoveList mls;
    EXPECT_TRUE(Pathfinding::CalculateMoveList(mls, path, speed_x, speed_y, 0, params));
    return mls;
}

// Steps forward until the move list is done; returns the positions after each step
std::vector<Point> WalkToEnd(MoveList &mls, int max_steps = 1000)
{
    std::vector<Point> positions;
    for (int i = 0; i < max_steps && !mls.IsDone(); ++i)
    {
        mls.Forward();
        positions.push_back(mls.GetCurrentPos());
    }
    return positions;
}

} // namespace

TEST(MoveList, HorizontalAndVerticalStages)
{
    MoveList mls = MakeMoveList({{0, 0}, {10, 0}, {10, -6}}, 2.f, 3.f);
    ASSERT_EQ(3u, mls.GetNumStages());
    EXPECT_FLOAT_EQ(2.f, mls.GetStageSpeed(0).X);
    EXPECT_FLOAT_EQ(0.f, mls.GetStageSpeed(0).Y);
    EXPECT_FLOAT_EQ(0.f, mls.GetStageSpeed(1).X);
    EXPECT_FLOAT_EQ(-3.f, mls.GetStageSpeed(1).Y);
    EXPECT_EQ(Point(0, 0), mls.GetCurrentPos());

    const std::vector<Point> walked = WalkToEnd(mls);
    const std::vector<Point> expected = {{2, 0}, {4, 0}, {6, 0}, {8, 0}, {10, 0}, {10, -3}, {10, -6}};
    EXPECT_EQ(expected, walked);
    EXPECT_TRUE(mls.IsDone());
    EXPECT_EQ(Point(10, -6), mls.GetCurrentPos());
}

TEST(MoveList, DiagonalStageWithEqualSpeeds)
{
    // a 3-4-5 triangle: speed 5 along the line is 3 and 4 along the axes
    MoveList mls = MakeMoveList({{0, 0}, {30, 40}}, 5.f, 5.f);
    EXPECT_NEAR(3.f, mls.GetStageSpeed(0).X, 1e-4);
    EXPECT_NEAR(4.f, mls.GetStageSpeed(0).Y, 1e-4);
    const std::vector<Point> walked = WalkToEnd(mls);
    ASSERT_EQ(10u, walked.size());
    EXPECT_EQ(Point(15, 20), walked[4]);
    EXPECT_EQ(Point(30, 40), walked.back());
}

TEST(MoveList, DiagonalStageWithDifferentSpeeds)
{
    // speed x 4, y 1: the speed along the line lies on the ellipse with these axes
    MoveList mls = MakeMoveList({{100, 100}, {60, 60}}, 4.f, 1.f);
    const Pointf speed = mls.GetStageSpeed(0);
    EXPECT_LT(speed.X, 0.f);
    EXPECT_LT(speed.Y, 0.f);
    // a 45 degree line: both components are equal
    EXPECT_NEAR(speed.X, speed.Y, 1e-4);
    const float along = std::hypot(speed.X, speed.Y);
    EXPECT_GE(along, 1.f);
    EXPECT_LE(along, 4.f);
    // (x/a)^2 + (y/b)^2 = 1
    EXPECT_NEAR(1.f, (speed.X / 4.f) * (speed.X / 4.f) + speed.Y * speed.Y, 1e-4);
    const std::vector<Point> walked = WalkToEnd(mls);
    EXPECT_EQ(Point(60, 60), walked.back());
}

TEST(MoveList, NegativeSpeedIsAFraction)
{
    // -4 means a quarter of a pixel per step
    MoveList mls = MakeMoveList({{0, 0}, {0, 3}}, -4.f, -4.f);
    EXPECT_FLOAT_EQ(0.25f, mls.GetStageSpeed(0).Y);
    const std::vector<Point> walked = WalkToEnd(mls);
    ASSERT_EQ(12u, walked.size());
    EXPECT_EQ(Point(0, 0), walked[2]);
    EXPECT_EQ(Point(0, 1), walked[3]);
    EXPECT_EQ(Point(0, 3), walked.back());
}

TEST(MoveList, LeftoverStepCarriesIntoTheNextStage)
{
    // 5 px at speed 2: the third step overshoots by 1 px, which is carried into the next stage
    MoveList mls = MakeMoveList({{0, 0}, {5, 0}, {5, 10}}, 2.f, 2.f);
    const std::vector<Point> walked = WalkToEnd(mls);
    const std::vector<Point> expected = {{2, 0}, {4, 0}, {5, 0}, {5, 3}, {5, 5}, {5, 7}, {5, 9}, {5, 10}};
    EXPECT_EQ(expected, walked);
}

TEST(MoveList, SpeedsPerStage)
{
    MoveList mls;
    ASSERT_TRUE(Pathfinding::CalculateMoveList(mls, {{0, 0}, {6, 0}, {6, 6}}, {{3.f, 3.f}, {1.f, 1.f}}, 0,
        RunPathParams::DefaultOnce()));
    EXPECT_FLOAT_EQ(3.f, mls.GetStageSpeed(0).X);
    EXPECT_FLOAT_EQ(1.f, mls.GetStageSpeed(1).Y);
    EXPECT_EQ(2u + 6u, WalkToEnd(mls).size());
}

// Debug builds assert on this input
#ifdef NDEBUG
TEST(MoveList, InvalidInputIsRefused)
{
    MoveList mls;
    EXPECT_FALSE(Pathfinding::CalculateMoveList(mls, {}, 1.f, 1.f, 0, RunPathParams::DefaultOnce()));
    // both speeds zero: no movement possible
    EXPECT_FALSE(Pathfinding::CalculateMoveList(mls, {{0, 0}, {1, 1}}, 0.f, 0.f, 0, RunPathParams::DefaultOnce()));
    // fewer speeds than stages
    EXPECT_FALSE(Pathfinding::CalculateMoveList(mls, {{0, 0}, {6, 0}, {6, 6}}, {{3.f, 3.f}}, 0,
        RunPathParams::DefaultOnce()));
}
#endif // NDEBUG

TEST(MoveList, SinglePointPathIsDoneAfterOneStep)
{
    MoveList mls = MakeMoveList({{7, 7}}, 1.f, 1.f);
    EXPECT_EQ(2u, mls.GetNumStages());
    const std::vector<Point> walked = WalkToEnd(mls);
    EXPECT_EQ(1u, walked.size());
    EXPECT_EQ(Point(7, 7), mls.GetCurrentPos());
}

TEST(MoveList, StageFlagsMarkDirectMovement)
{
    MoveList mls;
    ASSERT_TRUE(Pathfinding::CalculateMoveList(mls, {{0, 0}, {4, 0}}, 1.f, 1.f, kMoveStage_Direct,
        RunPathParams::DefaultOnce()));
    EXPECT_TRUE(mls.IsStageDirect());
    ASSERT_TRUE(Pathfinding::AddWaypointDirect(mls, 4, 4, 2.f, 2.f, 0));
    ASSERT_EQ(3u, mls.GetNumStages());
    EXPECT_FLOAT_EQ(2.f, mls.GetStageSpeed(1).Y);
    WalkToEnd(mls);
    EXPECT_EQ(Point(4, 4), mls.GetCurrentPos());
}

TEST(MoveList, AddWaypointToAnEmptyList)
{
    MoveList mls;
    ASSERT_TRUE(Pathfinding::AddWaypointDirect(mls, 3, 0, 1.f, 1.f, kMoveStage_Direct));
    ASSERT_EQ(2u, mls.GetNumStages());
    EXPECT_EQ(Point(3, 0), mls.GetStagePos(0));
    EXPECT_EQ(Point(3, 0), mls.GetStagePos(1));
}

TEST(MoveList, SpeedChangeKeepsThePosition)
{
    MoveList mls = MakeMoveList({{0, 0}, {20, 0}}, 1.f, 1.f);
    for (int i = 0; i < 6; ++i)
        mls.Forward();
    EXPECT_EQ(Point(6, 0), mls.GetCurrentPos());
    EXPECT_NEAR(0.3f, mls.GetStageProgress(), 1e-4);
    Pathfinding::RecalculateMoveSpeeds(mls, 1.f, 1.f, 2.f, 2.f);
    EXPECT_FLOAT_EQ(2.f, mls.GetStageSpeed(0).X);
    EXPECT_EQ(Point(6, 0), mls.GetCurrentPos());
    EXPECT_NEAR(0.3f, mls.GetStageProgress(), 1e-4);
    mls.Forward();
    EXPECT_EQ(Point(8, 0), mls.GetCurrentPos());
}

TEST(MoveList, SpeedChangeOnADiagonalWithDifferentSpeeds)
{
    MoveList mls = MakeMoveList({{0, 0}, {40, 40}}, 4.f, 1.f);
    for (int i = 0; i < 5; ++i)
        mls.Forward();
    const Point before = mls.GetCurrentPos();
    const float progress = mls.GetStageProgress();
    Pathfinding::RecalculateMoveSpeeds(mls, 4.f, 1.f, 8.f, 2.f);
    EXPECT_EQ(before, mls.GetCurrentPos());
    EXPECT_NEAR(progress, mls.GetStageProgress(), 1e-4);
    // the new speed is on the ellipse with axes 8 and 2
    const Pointf speed = mls.GetStageSpeed(0);
    EXPECT_NEAR(1.f, (speed.X / 8.f) * (speed.X / 8.f) + (speed.Y / 2.f) * (speed.Y / 2.f), 1e-4);
}

TEST(MoveList, RepeatStartsOverAtTheEnd)
{
    MoveList mls = MakeMoveList({{0, 0}, {3, 0}}, 1.f, 1.f, RunPathParams(kAnimFlow_Repeat, kAnimDirForward));
    std::vector<Point> walked;
    for (int i = 0; i < 7; ++i)
    {
        mls.Forward();
        walked.push_back(mls.GetCurrentPos());
    }
    // the step that arrives at the end lands on the start: the last point is never shown
    const std::vector<Point> expected = {{1, 0}, {2, 0}, {0, 0}, {1, 0}, {2, 0}, {0, 0}, {1, 0}};
    EXPECT_EQ(expected, walked);
    EXPECT_FALSE(mls.IsDone());
}

TEST(MoveList, OnceAndBackReturnsToTheStart)
{
    MoveList mls = MakeMoveList({{0, 0}, {3, 0}}, 1.f, 1.f, RunPathParams(kAnimFlow_OnceAndBack, kAnimDirForward));
    const std::vector<Point> walked = WalkToEnd(mls);
    const std::vector<Point> expected = {{1, 0}, {2, 0}, {3, 0}, {2, 0}, {1, 0}, {0, 0}};
    EXPECT_EQ(expected, walked);
    EXPECT_TRUE(mls.IsDone());
}

TEST(MoveList, OnceAndResetEndsAtTheStart)
{
    MoveList mls = MakeMoveList({{0, 0}, {3, 0}}, 1.f, 1.f, RunPathParams(kAnimFlow_OnceAndReset, kAnimDirForward));
    WalkToEnd(mls);
    EXPECT_TRUE(mls.IsDone());
    EXPECT_EQ(Point(0, 0), mls.GetCurrentPos());
}

TEST(MoveList, BackwardsWalksFromTheEnd)
{
    MoveList mls = MakeMoveList({{0, 0}, {4, 0}, {4, 4}}, 1.f, 1.f, RunPathParams(kAnimFlow_Once, kAnimDirBackward));
    EXPECT_EQ(Point(4, 4), mls.GetCurrentPos());
    const std::vector<Point> walked = WalkToEnd(mls);
    const std::vector<Point> expected = {{4, 3}, {4, 2}, {4, 1}, {4, 0}, {3, 0}, {2, 0}, {1, 0}, {0, 0}};
    EXPECT_EQ(expected, walked);
}

TEST(MoveList, BackwardStepRevertsTheProgress)
{
    MoveList mls = MakeMoveList({{0, 0}, {5, 0}, {5, 5}}, 1.f, 1.f);
    for (int i = 0; i < 7; ++i)
        mls.Forward();
    EXPECT_EQ(Point(5, 2), mls.GetCurrentPos());
    mls.Backward();
    EXPECT_EQ(Point(5, 1), mls.GetCurrentPos());
    mls.Backward();
    EXPECT_EQ(1u, mls.GetStage());
    EXPECT_EQ(Point(5, 0), mls.GetCurrentPos());
    // going back past the stage's start reverts to the end of the previous stage (the same point)
    mls.Backward();
    EXPECT_EQ(0u, mls.GetStage());
    EXPECT_EQ(Point(5, 0), mls.GetCurrentPos());
    mls.Backward();
    EXPECT_EQ(Point(4, 0), mls.GetCurrentPos());
}

TEST(MoveList, ResetToStageAndProgress)
{
    MoveList mls = MakeMoveList({{0, 0}, {10, 0}, {10, 10}}, 1.f, 1.f);
    mls.ResetToStage(1, 0.5f);
    EXPECT_EQ(1u, mls.GetStage());
    EXPECT_EQ(Point(10, 5), mls.GetCurrentPos());
    mls.SetStageProgress(0.2f);
    EXPECT_EQ(Point(10, 2), mls.GetCurrentPos());
    mls.ResetToEnd();
    EXPECT_EQ(Point(10, 10), mls.GetCurrentPos());
    mls.ResetToBegin();
    EXPECT_EQ(Point(0, 0), mls.GetCurrentPos());
    // stages past the last moving one are clamped to it
    mls.ResetToStage(5, 0.f);
    EXPECT_EQ(1u, mls.GetStage());
}

TEST(MoveList, ProgressOfABackwardRun)
{
    MoveList mls = MakeMoveList({{0, 0}, {10, 0}}, 1.f, 1.f, RunPathParams(kAnimFlow_Once, kAnimDirBackward));
    for (int i = 0; i < 3; ++i)
        mls.Forward();
    EXPECT_EQ(Point(7, 0), mls.GetCurrentPos());
    EXPECT_NEAR(0.3f, mls.GetStageProgress(), 1e-4);
    Pathfinding::RecalculateMoveSpeeds(mls, 1.f, 1.f, 2.f, 2.f);
    EXPECT_EQ(Point(7, 0), mls.GetCurrentPos());
    mls.Forward();
    EXPECT_EQ(Point(5, 0), mls.GetCurrentPos());
}
