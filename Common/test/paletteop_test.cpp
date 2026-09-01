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
#include "gtest/gtest.h"
#include "gfx/bitmapdata.h"

using namespace AGS::Common;

TEST(PixelOp, SetRGB) {
    Palette pal;
    std::fill(pal.begin(), pal.end(), RGB{0,0,0,0});
    PaletteOp::SetRGB(pal, 4, 255, 128, 64, 32);
    EXPECT_EQ(pal[4].r, 255);
    EXPECT_EQ(pal[4].g, 128);
    EXPECT_EQ(pal[4].b, 64);
    EXPECT_EQ(pal[4].a, 32);
}

TEST(PixelOp, Rotate) {
    Palette pal;
    std::fill(pal.begin(), pal.end(), RGB{0,0,0,0});
    PaletteOp::SetRGB(pal, 10, 1, 1, 1);
    PaletteOp::SetRGB(pal, 11, 2, 2, 2);
    PaletteOp::SetRGB(pal, 12, 3, 3, 3);
    PaletteOp::SetRGB(pal, 13, 4, 4, 4);
    PaletteOp::SetRGB(pal, 14, 5, 5, 5);

    PaletteOp::Rotate(pal, 10, 14, true /* left */);
    EXPECT_EQ(pal[10].r, 2);
    EXPECT_EQ(pal[11].r, 3);
    EXPECT_EQ(pal[12].r, 4);
    EXPECT_EQ(pal[13].r, 5);
    EXPECT_EQ(pal[14].r, 1);
    PaletteOp::Rotate(pal, 10, 14, true /* left */);
    EXPECT_EQ(pal[10].r, 3);
    EXPECT_EQ(pal[11].r, 4);
    EXPECT_EQ(pal[12].r, 5);
    EXPECT_EQ(pal[13].r, 1);
    EXPECT_EQ(pal[14].r, 2);
    PaletteOp::Rotate(pal, 10, 14, false /* right */);
    EXPECT_EQ(pal[10].r, 2);
    EXPECT_EQ(pal[11].r, 3);
    EXPECT_EQ(pal[12].r, 4);
    EXPECT_EQ(pal[13].r, 5);
    EXPECT_EQ(pal[14].r, 1);
    PaletteOp::Rotate(pal, 10, 14, false /* right */);
    EXPECT_EQ(pal[10].r, 1);
    EXPECT_EQ(pal[11].r, 2);
    EXPECT_EQ(pal[12].r, 3);
    EXPECT_EQ(pal[13].r, 4);
    EXPECT_EQ(pal[14].r, 5);
}
