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
#include <cstring>
#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include <allegro.h>
#include "gfx/image_file.h"
#include "util/memorystream.h"

using namespace AGS::Common;

// Sets the 32-bit color component shifts for the duration of a test
struct RGBShifts32
{
    RGBShifts32(int r, int g, int b, int a)
        : _r(_rgb_r_shift_32), _g(_rgb_g_shift_32), _b(_rgb_b_shift_32), _a(_rgb_a_shift_32)
    {
        _rgb_r_shift_32 = r; _rgb_g_shift_32 = g; _rgb_b_shift_32 = b; _rgb_a_shift_32 = a;
    }
    ~RGBShifts32()
    {
        _rgb_r_shift_32 = _r; _rgb_g_shift_32 = _g; _rgb_b_shift_32 = _b; _rgb_a_shift_32 = _a;
    }
private:
    int _r, _g, _b, _a;
};

static PixelBuffer SaveAndLoadPNG(const PixelBuffer &src, bool skip_alpha)
{
    std::vector<uint8_t> buf;
    {
        Stream out(std::make_unique<VectorStream>(buf, kStream_Write));
        EXPECT_TRUE(ImageFile::SavePNG(src, skip_alpha, nullptr, &out));
    }
    Stream in(std::make_unique<VectorStream>(buf));
    PixelFormat src_fmt = kPxFmt_Undefined;
    return ImageFile::LoadPNG(&in, &src_fmt, nullptr);
}

// With the pixel layout PNG uses (R,G,B,A in memory) LoadPNG copies the pixels as they are,
// instead of swapping components; it used to copy only width * height bytes then.
TEST(ImageFile, LoadPNG_RGBA_NoSwap)
{
    RGBShifts32 shifts(0, 8, 16, 24);
    const int w = 7, h = 5;
    PixelBuffer src(w, h, kPxFmt_A8R8G8B8);
    for (size_t i = 0; i < src.GetDataSize(); ++i)
        src.GetData()[i] = static_cast<uint8_t>(i * 7 + 1);

    PixelBuffer dst = SaveAndLoadPNG(src, false);
    ASSERT_TRUE(dst);
    ASSERT_EQ(dst.GetWidth(), w);
    ASSERT_EQ(dst.GetHeight(), h);
    ASSERT_EQ(dst.GetFormat(), kPxFmt_A8R8G8B8);
    ASSERT_EQ(dst.GetDataSize(), src.GetDataSize());
    EXPECT_EQ(std::memcmp(dst.GetData(), src.GetData(), src.GetDataSize()), 0);
}

TEST(ImageFile, LoadPNG_RGB_NoSwap)
{
    RGBShifts32 shifts(0, 8, 16, 24);
    const int w = 7, h = 5;
    PixelBuffer src(w, h, kPxFmt_R8G8B8);
    for (size_t i = 0; i < src.GetDataSize(); ++i)
        src.GetData()[i] = static_cast<uint8_t>(i * 7 + 1);

    PixelBuffer dst = SaveAndLoadPNG(src, false);
    ASSERT_TRUE(dst);
    ASSERT_EQ(dst.GetFormat(), kPxFmt_R8G8B8);
    ASSERT_EQ(dst.GetDataSize(), src.GetDataSize());
    EXPECT_EQ(std::memcmp(dst.GetData(), src.GetData(), src.GetDataSize()), 0);
}
