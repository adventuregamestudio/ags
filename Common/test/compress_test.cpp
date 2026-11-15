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
#include "gtest/gtest.h"
#include "util/lzw.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

using namespace AGS::Common;

TEST(Compress, LzwRoundTrip)
{
    // repetitive data, so that the compressor emits back references
    std::vector<uint8_t> data;
    for (int i = 0; i < 3000; ++i)
        data.push_back(static_cast<uint8_t>((i % 37) * 7 + (i / 400)));
    std::vector<uint8_t> packed;
    {
        Stream in(std::make_unique<VectorStream>(data));
        Stream out(std::make_unique<VectorStream>(packed, kStream_Write));
        ASSERT_TRUE(lzwcompress(&in, &out));
    }
    ASSERT_LT(packed.size(), data.size());

    std::vector<uint8_t> unpacked(data.size());
    ASSERT_TRUE(lzwexpand(packed.data(), packed.size(), unpacked.data(), unpacked.size()));
    EXPECT_EQ(data, unpacked);
}

TEST(Compress, LzwBackReferenceIsLittleEndian)
{
    // 32 equal bytes: a literal, then a reference of the longest match
    std::vector<uint8_t> data(32, 0x55);
    std::vector<uint8_t> packed;
    {
        Stream in(std::make_unique<VectorStream>(data));
        Stream out(std::make_unique<VectorStream>(packed, kStream_Write));
        ASSERT_TRUE(lzwcompress(&in, &out));
    }
    // flags (the second item is a reference), the literal, then 16-bit LE: ((match - 3) << 12) | distance
    ASSERT_GE(packed.size(), 4u);
    EXPECT_EQ(packed[0] & 0x03, 0x02);
    EXPECT_EQ(packed[1], 0x55);
    const uint16_t ref = packed[2] | (packed[3] << 8);
    EXPECT_EQ(ref >> 12, 13u); // the longest match: F - THRESHOLD
}
