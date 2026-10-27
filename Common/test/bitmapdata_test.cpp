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
#include <string.h>
#include "gtest/gtest.h"
#include "gfx/bitmapdata.h"

using namespace AGS::Common;

// Fills the buffer with a value unique to each pixel: (y * width + x + 1)
static void FillIndexed8(PixelBuffer &buf)
{
    for (int y = 0; y < buf.GetHeight(); ++y)
        for (int x = 0; x < buf.GetWidth(); ++x)
            buf.GetData()[y * buf.GetStride() + x] = static_cast<uint8_t>(y * buf.GetWidth() + x + 1);
}

static void Fill32(PixelBuffer &buf)
{
    for (int y = 0; y < buf.GetHeight(); ++y)
        for (int x = 0; x < buf.GetWidth(); ++x)
            reinterpret_cast<uint32_t*>(buf.GetData() + y * buf.GetStride())[x] = 0xFF000000u | (y << 8) | x;
}

TEST(PixelOp, CopyPixelsRegionWithSourceOffset) {
    PixelBuffer src(5, 4, kPxFmt_Indexed8);
    FillIndexed8(src);

    // region below the first row
    PixelBuffer out = PixelOp::CopyPixelsRegion(src, 1, 2, 3, 2);
    ASSERT_EQ(out.GetWidth(), 3);
    ASSERT_EQ(out.GetHeight(), 2);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 3; ++x)
            EXPECT_EQ(out.GetData()[y * out.GetStride() + x], (y + 2) * 5 + (x + 1) + 1);
}

TEST(PixelOp, CopyPixelsRegionWithSourceOffset32) {
    PixelBuffer src(4, 3, kPxFmt_A8R8G8B8);
    Fill32(src);

    PixelBuffer out = PixelOp::CopyPixelsRegion(src, 2, 1, 2, 2);
    ASSERT_EQ(out.GetWidth(), 2);
    ASSERT_EQ(out.GetHeight(), 2);
    for (int y = 0; y < 2; ++y)
        for (int x = 0; x < 2; ++x)
            EXPECT_EQ(reinterpret_cast<const uint32_t*>(out.GetData() + y * out.GetStride())[x],
                0xFF000000u | ((y + 1) << 8) | (x + 2));
}

TEST(PixelOp, CopyPixelsRegionWithDestOffset) {
    PixelBuffer src(2, 2, kPxFmt_Indexed8);
    FillIndexed8(src); // 1 2 / 3 4
    PixelBuffer dst(4, 4, kPxFmt_Indexed8);
    memset(dst.GetData(), 0, dst.GetDataSize());

    PixelOp::CopyPixelsRegion(src.GetData(), 1, src.GetStride(), 0, 0, 2, 2,
        dst.GetData(), dst.GetStride(), 1, 2);
    const uint8_t expect[4][4] = {
        { 0, 0, 0, 0 },
        { 0, 0, 0, 0 },
        { 0, 1, 2, 0 },
        { 0, 3, 4, 0 } };
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            EXPECT_EQ(dst.GetData()[y * dst.GetStride() + x], expect[y][x]) << "at " << x << "," << y;
}

TEST(PixelOp, CopyPixelsRegionFullRowsWithOffset) {
    // full width: the source and dest scan lines match the copied width
    PixelBuffer src(4, 4, kPxFmt_Indexed8);
    FillIndexed8(src);

    PixelBuffer out = PixelOp::CopyPixelsRegion(src, 0, 1, 4, 3);
    ASSERT_EQ(out.GetHeight(), 3);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 4; ++x)
            EXPECT_EQ(out.GetData()[y * out.GetStride() + x], (y + 1) * 4 + x + 1);
}
