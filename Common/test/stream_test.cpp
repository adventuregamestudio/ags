#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include "util/alignedstream.h"
#include "util/memorystream.h"
#include "util/string_utils.h"

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

TEST(Stream, Common) {
    // Storage buffer
    std::vector<char> membuf;

    //-------------------------------------------------------------------------
    // Write data
    std::unique_ptr<Stream> out(
        new MemoryStream(membuf, kStream_Write));

    out->WriteInt16(10);
    out->WriteInt64(-20202);
    StrUtil::WriteCStr("test.tmp", out.get());
    String very_long_string;
    very_long_string.FillString('a', 10000);
    very_long_string.Write(out.get());

    out.reset();

    //-------------------------------------------------------------------------
    // Read data back
    std::unique_ptr<Stream> in(
        new MemoryStream(membuf));

    int16_t int16val = in->ReadInt16();
    int64_t int64val = in->ReadInt64();
    String str1 = String::FromStream(in.get());
    String str2 = String::FromStream(in.get());

    in.reset();

    //-----------------------------------------------------
    // Assertions

    ASSERT_TRUE(int16val == 10);
    ASSERT_TRUE(int64val == -20202);
    ASSERT_TRUE(strcmp(str1.GetCStr(), "test.tmp") == 0);
    ASSERT_TRUE(strcmp(str2.GetCStr(), very_long_string.GetCStr()) == 0);
}

TEST(Stream, AlignedStream) {
    // Storage buffer
    std::vector<char> membuf;

    //-------------------------------------------------------------------------
    // Write data
    std::unique_ptr<Stream> out(
        new MemoryStream(membuf, kStream_Write));

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
        bigend_data.b = BBOp::SwapBytesInt32(bigend_data.b);
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

    out.reset();

    //-------------------------------------------------------------------------
    // Read data back
    std::unique_ptr<Stream> in(
        new MemoryStream(membuf));

    TTrickyAlignedData tricky_data_in;
    memset(&tricky_data_in, 0xAA, sizeof(tricky_data_in));
    {
        AlignedStream as(in.get(), AGS::Common::kAligned_Read);
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

    int32_t int32val = in->ReadInt32();

    in.reset();

    //-----------------------------------------------------
    // Assertions

    ASSERT_EQ(tricky_data_out.a, tricky_data_in.a);
    ASSERT_EQ(tricky_data_out.b, tricky_data_in.b);
    ASSERT_EQ(tricky_data_out.d[0], tricky_data_in.d[0]);
    ASSERT_EQ(tricky_data_out.d[1], tricky_data_in.d[1]);
    ASSERT_EQ(tricky_data_out.d[2], tricky_data_in.d[2]);
    ASSERT_EQ(tricky_data_out.e, tricky_data_in.e);
    ASSERT_EQ(tricky_data_out.f[0], tricky_data_in.f[0]);
    ASSERT_EQ(tricky_data_out.g[0], tricky_data_in.g[0]);
    ASSERT_EQ(tricky_data_out.h[0], tricky_data_in.h[0]);
    ASSERT_EQ(tricky_data_out.i[0], tricky_data_in.i[0]);
    ASSERT_EQ(tricky_data_out.j, tricky_data_in.j);
    ASSERT_EQ(tricky_data_out.k, tricky_data_in.k);
    ASSERT_EQ(tricky_data_out.l, tricky_data_in.l);
    ASSERT_EQ(tricky_data_out.m, tricky_data_in.m);
    ASSERT_EQ(tricky_data_out.n, tricky_data_in.n);
    ASSERT_EQ(tricky_data_out.i64a, tricky_data_in.i64a);
    ASSERT_EQ(tricky_data_out.o, tricky_data_in.o);
    ASSERT_EQ(tricky_data_out.i64b, tricky_data_in.i64b);
    ASSERT_EQ(tricky_data_out.p, tricky_data_in.p);
    ASSERT_EQ(tricky_data_out.i64c, tricky_data_in.i64c);
    ASSERT_EQ(tricky_data_out.q, tricky_data_in.q);
    ASSERT_EQ(tricky_data_out.r, tricky_data_in.r);
    ASSERT_EQ(tricky_data_out.i64d, tricky_data_in.i64d);
    ASSERT_EQ(tricky_data_out.final, tricky_data_in.final);
    ASSERT_TRUE(memcmp(&tricky_data_in, &tricky_data_out, sizeof(TTrickyAlignedData)) == 0);
    ASSERT_TRUE(int32val == 20);
}
