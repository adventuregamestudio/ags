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
//
// Unit tests for LOCTYPE_EDGE detection in GetLocationType
//
//=============================================================================
#include "gtest/gtest.h"
#include "ac/runtime_defines.h"

// Edge detection logic - mirrors the edge detection in __GetLocationType()
// Returns edge index (0=left, 1=right, 2=bottom, 3=top) or -1 if not on edge
static int detect_edge(int x, int y, int left, int right, int top, int bottom)
{
    if (x <= left)
        return 0; // Left edge
    else if (x >= right)
        return 1; // Right edge
    else if (y >= bottom)
        return 2; // Bottom edge
    else if (y <= top)
        return 3; // Top edge
    return -1; // Not on any edge
}

// Test that LOCTYPE_EDGE constant is defined correctly
TEST(EdgeLocationType, ConstantValue) {
    EXPECT_EQ(LOCTYPE_EDGE, 5);
}

// Test edge detection for each edge
TEST(EdgeLocationType, DetectsLeftEdge) {
    const int left = 50, right = 270, top = 40, bottom = 180;
    EXPECT_EQ(detect_edge(50, 100, left, right, top, bottom), 0);
    EXPECT_EQ(detect_edge(30, 100, left, right, top, bottom), 0);
}

TEST(EdgeLocationType, DetectsRightEdge) {
    const int left = 50, right = 270, top = 40, bottom = 180;
    EXPECT_EQ(detect_edge(270, 100, left, right, top, bottom), 1);
    EXPECT_EQ(detect_edge(300, 100, left, right, top, bottom), 1);
}

TEST(EdgeLocationType, DetectsBottomEdge) {
    const int left = 50, right = 270, top = 40, bottom = 180;
    EXPECT_EQ(detect_edge(150, 180, left, right, top, bottom), 2);
    EXPECT_EQ(detect_edge(150, 200, left, right, top, bottom), 2);
}

TEST(EdgeLocationType, DetectsTopEdge) {
    const int left = 50, right = 270, top = 40, bottom = 180;
    EXPECT_EQ(detect_edge(150, 40, left, right, top, bottom), 3);
    EXPECT_EQ(detect_edge(150, 20, left, right, top, bottom), 3);
}

TEST(EdgeLocationType, InsideRoomReturnsNoEdge) {
    const int left = 50, right = 270, top = 40, bottom = 180;
    EXPECT_EQ(detect_edge(150, 100, left, right, top, bottom), -1);
    EXPECT_EQ(detect_edge(51, 100, left, right, top, bottom), -1);
    EXPECT_EQ(detect_edge(269, 100, left, right, top, bottom), -1);
}

TEST(EdgeLocationType, CornerPriorityLeftRightFirst) {
    const int left = 50, right = 270, top = 40, bottom = 180;
    // Corners: left/right edges take priority over top/bottom
    EXPECT_EQ(detect_edge(50, 40, left, right, top, bottom), 0);   // top-left -> left
    EXPECT_EQ(detect_edge(270, 40, left, right, top, bottom), 1);  // top-right -> right
    EXPECT_EQ(detect_edge(50, 180, left, right, top, bottom), 0);  // bottom-left -> left
    EXPECT_EQ(detect_edge(270, 180, left, right, top, bottom), 1); // bottom-right -> right
}
