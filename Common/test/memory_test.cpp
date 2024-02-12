//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2024 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "gtest/gtest.h"
#include "util/memory.h"

using namespace AGS::Common;

TEST(Memory, SwapBytes) {
    const int16_t i16 = (int16_t)0xABCD;
    const int32_t i32 = (int32_t)0xABCDEF12;
    const int64_t i64 = (int64_t)0xABCDEF1234567890;

    ASSERT_TRUE(BBOp::SwapBytesInt16(i16) == (int16_t)0xCDAB);
    ASSERT_TRUE(BBOp::SwapBytesInt32(i32) == (int32_t)0x12EFCDABu);
    ASSERT_TRUE(BBOp::SwapBytesInt64(i64) == (int64_t)0x9078563412EFCDABul);
}

TEST(Memory, BBOp) {
    const int16_t i16 = (int16_t)0xABCD;
    const int32_t i32 = (int32_t)0xABCDEF12;
    const int64_t i64 = (int64_t)0xABCDEF1234567890;

#if defined (BITBYTE_BIG_ENDIAN)
    ASSERT_TRUE(BBOp::Int16FromLE(i16) == (int16_t)0xCDAB);
    ASSERT_TRUE(BBOp::Int32FromLE(i32) == (int32_t)0x12EFCDABu);
    ASSERT_TRUE(BBOp::Int64FromLE(i64) == (int64_t)0x9078563412EFCDABul);

    ASSERT_TRUE(BBOp::Int16FromBE(i16) == (int16_t)0xABCD);
    ASSERT_TRUE(BBOp::Int32FromBE(i32) == (int32_t)0xABCDEF12);
    ASSERT_TRUE(BBOp::Int64FromBE(i64) == (int64_t)0xABCDEF1234567890);
#else
    ASSERT_TRUE(BBOp::Int16FromLE(i16) == (int16_t)0xABCD);
    ASSERT_TRUE(BBOp::Int32FromLE(i32) == (int32_t)0xABCDEF12);
    ASSERT_TRUE(BBOp::Int64FromLE(i64) == (int64_t)0xABCDEF1234567890);

    ASSERT_TRUE(BBOp::Int16FromBE(i16) == (int16_t)0xCDAB);
    ASSERT_TRUE(BBOp::Int32FromBE(i32) == (int32_t)0x12EFCDABu);
    ASSERT_TRUE(BBOp::Int64FromBE(i64) == (int64_t)0x9078563412EFCDABul);
#endif
}

TEST(Memory, ReadWrite) {
    int16_t dst_i16 = (int16_t)0xABCD;
    int32_t dst_i32 = (int32_t)0xABCDEF12;
    int64_t dst_i64 = (int64_t)0xABCDEF1234567890;
    void *p_i16 = &dst_i16;
    void *p_i32 = &dst_i32;
    void *p_i64 = &dst_i64;

#if defined (TEST_BIGENDIAN)
    dst_i16 = BBOp::SwapBytesInt16(dst_i16);
    dst_i32 = BBOp::SwapBytesInt32(dst_i32);
    dst_i64 = BBOp::SwapBytesInt64(dst_i64);
#endif

    ASSERT_TRUE(Memory::ReadInt16(p_i16) == (int16_t)0xABCD);
    ASSERT_TRUE(Memory::ReadInt32(p_i32) == (int32_t)0xABCDEF12);
    ASSERT_TRUE(Memory::ReadInt64(p_i64) == (int64_t)0xABCDEF1234567890);

    Memory::WriteInt16(p_i16, (int16_t)0xCDAB);
    Memory::WriteInt32(p_i32, (int32_t)0x12EFCDAB);
    Memory::WriteInt64(p_i64, (int64_t)0x9078563412EFCDAB);

#if defined (TEST_BIGENDIAN)
    dst_i16 = BBOp::SwapBytesInt16(dst_i16);
    dst_i32 = BBOp::SwapBytesInt32(dst_i32);
    dst_i64 = BBOp::SwapBytesInt64(dst_i64);
#endif

    ASSERT_TRUE(dst_i16 == (int16_t)0xCDAB);
    ASSERT_TRUE(dst_i32 == (int32_t)0x12EFCDAB);
    ASSERT_TRUE(dst_i64 == (int64_t)0x9078563412EFCDAB);
}
