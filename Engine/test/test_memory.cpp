//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#ifdef _DEBUG

#include "util/bbop.h"
#include "debug/assert.h"

using namespace AGS::Common;

void Test_Memory()
{
    int16_t i16 = (int16_t)0xABCD;
    int32_t i32 = (int32_t)0xABCDEF12;
    int64_t i64 = (int64_t)0xABCDEF1234567890;

    assert(BBOp::SwapBytesInt16(i16) == (int16_t)0xCDAB);
    assert(BBOp::SwapBytesInt32(i32) == (int32_t)0x12EFCDABu);
    assert(BBOp::SwapBytesInt64(i64) == (int64_t)0x9078563412EFCDABul);

#if defined (BITBYTE_BIG_ENDIAN)
    assert(BBOp::Int16FromLE(i16) == (int16_t)0xCDAB);
    assert(BBOp::Int32FromLE(i32) == (int32_t)0x12EFCDABu);
    assert(BBOp::Int64FromLE(i64) == (int64_t)0x9078563412EFCDABul);

    assert(BBOp::Int16FromBE(i16) == (int16_t)0xABCD);
    assert(BBOp::Int32FromBE(i32) == (int32_t)0xABCDEF12);
    assert(BBOp::Int64FromBE(i64) == (int64_t)0xABCDEF1234567890);
#else
    assert(BBOp::Int16FromLE(i16) == (int16_t)0xABCD);
    assert(BBOp::Int32FromLE(i32) == (int32_t)0xABCDEF12);
    assert(BBOp::Int64FromLE(i64) == (int64_t)0xABCDEF1234567890);

    assert(BBOp::Int16FromBE(i16) == (int16_t)0xCDAB);
    assert(BBOp::Int32FromBE(i32) == (int32_t)0x12EFCDABu);
    assert(BBOp::Int64FromBE(i64) == (int64_t)0x9078563412EFCDABul);
#endif
}

#endif // _DEBUG
