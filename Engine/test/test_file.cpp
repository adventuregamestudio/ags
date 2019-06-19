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

#include "core/platform.h"
#if AGS_PLATFORM_DEBUG

#include <string.h>
#include "debug/assert.h"
#include "util/stream.h"
#include "util/file.h"

using namespace AGS::Common;

struct TTrickyAlignedData
{
    char    a;
    int     b;
    int     c;
    short   d[3];
    int     e;
    char    f[17];
    int     g[4];
    short   h[13];
    char    i[3];
    short   j;
    int     k;
    short   l;
    short   m;
    int     n;
    int64_t i64a;
    char    o;
    int64_t i64b;
    short   p;
    int64_t i64c;
    short   q;
    short   r;
    int64_t i64d;
    char    final;
};

void Test_File()
{
    //-----------------------------------------------------
    // Operations
    Stream *out = File::OpenFile("test.tmp", AGS::Common::kFile_CreateAlways, AGS::Common::kFile_Write);

    out->WriteInt16(10);
    out->WriteInt64(-20202);
    String::WriteString("test.tmp", out);
    String very_long_string;
    very_long_string.FillString('a', 10000);
    very_long_string.Write(out);

    TTrickyAlignedData tricky_data_out;
    memset(&tricky_data_out, 0xAA, sizeof(tricky_data_out));
    {
        tricky_data_out.a = 11;
        tricky_data_out.b = 12;
        tricky_data_out.c = 13;
        tricky_data_out.d[0] = 14;
        tricky_data_out.d[1] = 15;
        tricky_data_out.d[2] = 16;
        tricky_data_out.e = 17;
        memset(tricky_data_out.f, 0, 17);
        tricky_data_out.g[0] = 18;
        tricky_data_out.g[1] = 19;
        tricky_data_out.g[2] = 20;
        tricky_data_out.g[3] = 21;
        memset(tricky_data_out.h, 0, 13 * sizeof(short));
        tricky_data_out.i[0] = 22;
        tricky_data_out.i[1] = 23;
        tricky_data_out.i[2] = 24;
        tricky_data_out.j = 25;
        tricky_data_out.k = 26;
        tricky_data_out.l = 27;
        tricky_data_out.m = 28;
        tricky_data_out.n = 29;
        tricky_data_out.i64a = 30;
        tricky_data_out.o = 31;
        tricky_data_out.i64b = 32;
        tricky_data_out.p = 33;
        tricky_data_out.i64c = 34;
        tricky_data_out.q = 35;
        tricky_data_out.r = 36;
        tricky_data_out.i64d = 37;
        tricky_data_out.final = 38;
#if defined (TEST_BIGENDIAN)
        TTrickyAlignedData bigend_data = tricky_data_out;
        bigend_data.b =BBOp::SwapBytesInt32(bigend_data.b);
        bigend_data.c = BBOp::SwapBytesInt32(bigend_data.c);
        for (int i = 0; i < 3; ++i)
        {
            bigend_data.d[i] = BBOp::SwapBytesInt16(bigend_data.d[i]);
        }
        bigend_data.e = BBOp::SwapBytesInt32(bigend_data.e);
        for (int i = 0; i < 4; ++i)
        {
            bigend_data.g[i] = BBOp::SwapBytesInt32(bigend_data.g[i]);
        }
        for (int i = 0; i < 13; ++i)
        {
            bigend_data.h[i] = BBOp::SwapBytesInt16(bigend_data.h[i]);
        }
        bigend_data.j = BBOp::SwapBytesInt16(bigend_data.j);
        bigend_data.k = BBOp::SwapBytesInt32(bigend_data.k);
        bigend_data.l = BBOp::SwapBytesInt16(bigend_data.l);
        bigend_data.m = BBOp::SwapBytesInt16(bigend_data.m);
        bigend_data.n = BBOp::SwapBytesInt32(bigend_data.n);
        bigend_data.i64a = BBOp::SwapBytesInt64(bigend_data.i64a);
        bigend_data.i64b = BBOp::SwapBytesInt64(bigend_data.i64b);
        bigend_data.p = BBOp::SwapBytesInt16(bigend_data.p);
        bigend_data.i64c = BBOp::SwapBytesInt64(bigend_data.i64c);
        bigend_data.q = BBOp::SwapBytesInt16(bigend_data.q);
        bigend_data.r = BBOp::SwapBytesInt16(bigend_data.r);
        bigend_data.i64d = BBOp::SwapBytesInt64(bigend_data.i64d);
        out->Write(&bigend_data, sizeof(TTrickyAlignedData));
#else
        out->Write(&tricky_data_out, sizeof(TTrickyAlignedData));
#endif
    }

    out->WriteInt32(20);

    delete out;

    //-------------------------------------------------------------------------

    Stream *in = File::OpenFile("test.tmp", AGS::Common::kFile_Open, AGS::Common::kFile_Read);

    int16_t int16val    = in->ReadInt16();
    int64_t int64val    = in->ReadInt64();
    String str1         = String::FromStream(in);
    String str2         = String::FromStream(in);

    TTrickyAlignedData tricky_data_in;
    memset(&tricky_data_in, 0xAA, sizeof(tricky_data_in));
    {
        AlignedStream as(in, AGS::Common::kAligned_Read);
        tricky_data_in.a = as.ReadInt8();
        tricky_data_in.b = as.ReadInt32();
        tricky_data_in.c = as.ReadInt32();
        as.ReadArrayOfInt16(tricky_data_in.d, 3);
        tricky_data_in.e = as.ReadInt32();
        as.Read(tricky_data_in.f, 17);
        as.ReadArrayOfInt32(tricky_data_in.g, 4);
        as.ReadArrayOfInt16(tricky_data_in.h, 13);
        as.Read(tricky_data_in.i, 3);
        tricky_data_in.j = as.ReadInt16();
        tricky_data_in.k = as.ReadInt32();
        tricky_data_in.l = as.ReadInt16();
        tricky_data_in.m = as.ReadInt16();
        tricky_data_in.n = as.ReadInt32();
        tricky_data_in.i64a = as.ReadInt64();
        tricky_data_in.o = as.ReadInt8();
        tricky_data_in.i64b = as.ReadInt64();
        tricky_data_in.p = as.ReadInt16();
        tricky_data_in.i64c = as.ReadInt64();
        tricky_data_in.q = as.ReadInt16();
        tricky_data_in.r = as.ReadInt16();
        tricky_data_in.i64d = as.ReadInt64();
        tricky_data_in.final = as.ReadInt8();
    }

    int32_t int32val    = in->ReadInt32();

    delete in;

    File::DeleteFile("test.tmp");

    //-----------------------------------------------------
    // Assertions
    assert(int16val == 10);
    assert(int64val == -20202);
    assert(strcmp(str1, "test.tmp") == 0);
    assert(strcmp(str2, very_long_string) == 0);
    assert(memcmp(&tricky_data_in, &tricky_data_out, sizeof(TTrickyAlignedData)) == 0);
    assert(int32val == 20);

    assert(!File::TestReadFile("test.tmp"));
}

#endif // AGS_PLATFORM_DEBUG
