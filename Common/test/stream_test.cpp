#include <array>
#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include "util/alignedstream.h"
#include "util/bufferedstream.h"
#include "util/memorystream.h"
#include "util/string_utils.h"

using namespace AGS::Common;

TEST(Stream, Common) {
    // Storage buffer
    std::vector<uint8_t> membuf;

    //-------------------------------------------------------------------------
    // Write data
    std::unique_ptr<Stream> out(
        new VectorStream(membuf, kStream_Write));

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
        new VectorStream(membuf));

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

TEST(Stream, MemoryStream) {
    // Storage buffer
    std::array<uint8_t, 1024> membuf;
    const size_t fill_len = 10;
    //-------------------------------------------------------------------------
    // Write data
    MemoryStream out(&membuf.front(), membuf.size(), kStream_Write);
    ASSERT_TRUE(out.CanWrite());
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    out.WriteInt32(3);
    out.WriteByteCount(0, fill_len);
    out.WriteInt32(4);
    out.WriteInt32(5);
    out.WriteInt32(6);
    out.WriteInt32(7);
    out.WriteByteCount(0, fill_len);
    out.WriteInt32(8);
    out.WriteInt32(9);
    out.WriteInt32(10);
    out.WriteInt32(11);
    const auto eos_pos = out.GetPosition();
    ASSERT_EQ(eos_pos, sizeof(int32_t) * 4 * 3 + fill_len * 2);
    ASSERT_EQ(out.GetLength(), sizeof(int32_t) * 4 * 3 + fill_len * 2);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    MemoryStream in(&membuf.front(), static_cast<size_t>(eos_pos), kStream_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_EQ(in.GetLength(), sizeof(int32_t) * 4 * 3 + fill_len * 2);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 3);
    for (size_t i = 0; i < fill_len; ++i)
        in.ReadByte();
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 6);
    ASSERT_EQ(in.ReadInt32(), 7);
    for (size_t i = 0; i < fill_len; ++i)
        in.ReadByte();
    ASSERT_EQ(in.ReadInt32(), 8);
    ASSERT_EQ(in.ReadInt32(), 9);
    ASSERT_EQ(in.ReadInt32(), 10);
    ASSERT_EQ(in.ReadInt32(), 11);
    ASSERT_EQ(in.GetPosition(), sizeof(int32_t) * 4 * 3 + fill_len * 2);
    ASSERT_TRUE(in.EOS());
    in.Close();
    //-------------------------------------------------------------------------
    // Test seeks
    MemoryStream in2(&membuf.front(), static_cast<size_t>(eos_pos), kStream_Read);
    ASSERT_TRUE(in2.CanRead());
    ASSERT_EQ(in2.GetLength(), sizeof(int32_t) * 4 * 3 + fill_len * 2);
    in2.Seek(4 * sizeof(int32_t) + fill_len, kSeekBegin);
    ASSERT_EQ(in2.ReadInt32(), 4);
    in2.Seek(2 * sizeof(int32_t), kSeekBegin);
    ASSERT_EQ(in2.ReadInt32(), 2);
    in2.Seek(8 * sizeof(int32_t) + fill_len * 2, kSeekBegin);
    ASSERT_EQ(in2.ReadInt32(), 8);
    in2.Seek(2 * sizeof(int32_t), kSeekCurrent);
    ASSERT_EQ(in2.ReadInt32(), 11);
    ASSERT_EQ(in2.GetPosition(), sizeof(int32_t) * 4 * 3 + fill_len * 2);
    ASSERT_TRUE(in2.EOS());
    in2.Close();
}

TEST(Stream, MemoryStream2) {
    // Storage buffer
    std::array<uint8_t, 1024> membuf;
    const size_t fill_len = 10;
    //-------------------------------------------------------------------------
    // Write data with seeks
    MemoryStream out(&membuf.front(), membuf.size(), kStream_Write);
    ASSERT_TRUE(out.CanWrite());
    ASSERT_TRUE(out.CanSeek());
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    const auto write_back_pos = out.GetPosition();
    out.WriteInt32(3);
    out.WriteByteCount(0, fill_len);
    out.Seek(write_back_pos, kSeekBegin);
    out.WriteInt32(111);
    out.Seek(0, kSeekEnd);
    out.WriteInt32(222);
    const auto eos_pos = out.GetPosition();
    ASSERT_EQ(eos_pos, sizeof(int32_t) * 5 + fill_len);
    ASSERT_EQ(out.GetLength(), sizeof(int32_t) * 5 + fill_len);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    MemoryStream in(&membuf.front(), static_cast<size_t>(eos_pos), kStream_Read);
    ASSERT_EQ(in.GetLength(), sizeof(int32_t) * 5 + fill_len);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 111);
    in.Seek(fill_len, kSeekCurrent);
    ASSERT_EQ(in.ReadInt32(), 222);
    ASSERT_EQ(in.GetPosition(), sizeof(int32_t) * 5 + fill_len);
    ASSERT_TRUE(in.EOS());
    in.Close();
}

TEST(Stream, VectorStream) {
    // Storage buffer
    std::vector<uint8_t> membuf;
    const size_t fill_len = 10;
    //-------------------------------------------------------------------------
    // Write data with seeks
    VectorStream out(membuf, kStream_Write);
    ASSERT_TRUE(out.CanWrite());
    ASSERT_TRUE(out.CanSeek());
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    auto write_back_pos = out.GetPosition();
    out.WriteInt32(3);
    out.WriteByteCount(0, fill_len);
    out.Seek(write_back_pos, kSeekBegin);
    out.WriteInt32(111);
    out.Seek(0, kSeekEnd);
    out.WriteInt32(222);
    ASSERT_EQ(out.GetPosition(), sizeof(int32_t) * 5 + fill_len);
    ASSERT_EQ(out.GetLength(), sizeof(int32_t) * 5 + fill_len);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    VectorStream in(membuf, kStream_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_TRUE(in.CanSeek());
    ASSERT_EQ(in.GetLength(), sizeof(int32_t) * 5 + fill_len);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 111);
    in.Seek(fill_len, kSeekCurrent);
    ASSERT_EQ(in.ReadInt32(), 222);
    ASSERT_EQ(in.GetPosition(), sizeof(int32_t) * 5 + fill_len);
    ASSERT_TRUE(in.EOS());
    in.Close();
}

#if (AGS_PLATFORM_TEST_FILE_IO)

static const char *DummyFile = "dummy.dat";

TEST(Stream, BufferedStreamRead) {
    //-------------------------------------------------------------------------
    // Write data into the temp file
    FileStream out(DummyFile, kFile_CreateAlways, kFile_Write);
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    out.WriteInt32(3);
    // fill in to ensure buffered stream reach buffer size
    out.WriteByteCount(0, BufferedStream::BufferSize);
    out.WriteInt32(4);
    out.WriteInt32(5);
    out.WriteInt32(6);
    out.WriteInt32(7);
    out.WriteByteCount(0, BufferedStream::BufferSize);
    out.WriteInt32(8);
    out.WriteInt32(9);
    out.WriteInt32(10);
    out.WriteInt32(11);
    const soff_t file_len = out.GetLength();
    out.Close();

    //-------------------------------------------------------------------------
    // Read data back
    BufferedStream in(DummyFile, kFile_Open, kFile_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_EQ(in.GetLength(), file_len);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 3);
    for (size_t i = 0; i < BufferedStream::BufferSize / 4; ++i)
        in.ReadInt32(); // skip
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 6);
    ASSERT_EQ(in.ReadInt32(), 7);
    for (size_t i = 0; i < BufferedStream::BufferSize / 4; ++i)
        in.ReadInt32(); // skip
    ASSERT_EQ(in.ReadInt32(), 8);
    ASSERT_EQ(in.ReadInt32(), 9);
    ASSERT_EQ(in.ReadInt32(), 10);
    ASSERT_EQ(in.ReadInt32(), 11);
    ASSERT_EQ(in.GetPosition(), file_len);
    ASSERT_TRUE(in.EOS());
    in.Close();

    // Test seeks
    BufferedStream in2(DummyFile, kFile_Open, kFile_Read);
    ASSERT_TRUE(in2.CanRead());
    ASSERT_TRUE(in2.CanSeek());
    ASSERT_EQ(in2.GetLength(), file_len);
    in2.Seek(4 * sizeof(int32_t) + BufferedStream::BufferSize, kSeekBegin);
    ASSERT_EQ(in2.ReadInt32(), 4);
    in2.Seek(2 * sizeof(int32_t), kSeekBegin);
    ASSERT_EQ(in2.ReadInt32(), 2);
    in2.Seek(8 * sizeof(int32_t) + BufferedStream::BufferSize * 2, kSeekBegin);
    ASSERT_EQ(in2.ReadInt32(), 8);
    in2.Seek(2 * sizeof(int32_t), kSeekCurrent);
    ASSERT_EQ(in2.ReadInt32(), 11);
    ASSERT_EQ(in2.GetPosition(), file_len);
    ASSERT_TRUE(in2.EOS());
    in2.Close();

    File::DeleteFile(DummyFile);
}

TEST(Stream, BufferedStreamWrite) {
    //-------------------------------------------------------------------------
    // Write data into the temp file
    // fill in to ensure buffered stream reach buffer size
    size_t fill_len = BufferedStream::BufferSize;
    //-------------------------------------------------------------------------
    // Write data
    const soff_t file_len = sizeof(int32_t) * 9 + fill_len + fill_len / 2;
    BufferedStream out(DummyFile, kFile_CreateAlways, kFile_Write);
    ASSERT_TRUE(out.CanWrite());
    ASSERT_TRUE(out.CanSeek());
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    auto write_back_pos = out.GetPosition();
    out.WriteInt32(3);
    out.WriteByteCount(0, fill_len);
    out.Seek(write_back_pos, kSeekBegin);
    out.WriteInt32(111);
    out.Seek(0, kSeekEnd);
    out.WriteInt32(4);
    out.WriteInt32(5);
    out.WriteInt32(6);
    write_back_pos = out.GetPosition();
    out.WriteInt32(7);
    out.WriteByteCount(0, fill_len / 2);
    out.Seek(write_back_pos, kSeekBegin);
    out.WriteInt32(222);
    out.Seek(0, kSeekEnd);
    out.WriteInt32(333);
    ASSERT_EQ(out.GetPosition(), file_len);
    ASSERT_EQ(out.GetLength(), file_len);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    FileStream in(DummyFile, kFile_Open, kFile_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_TRUE(in.CanSeek());
    ASSERT_EQ(in.GetLength(), file_len);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 111);
    in.Seek(fill_len, kSeekCurrent);
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 6);
    ASSERT_EQ(in.ReadInt32(), 222);
    in.Seek(fill_len / 2, kSeekCurrent);
    ASSERT_EQ(in.ReadInt32(), 333);
    ASSERT_EQ(in.GetPosition(), file_len);
    in.Close();

    File::DeleteFile(DummyFile);
}

TEST(Stream, BufferedSectionStream) {
    //-------------------------------------------------------------------------
    // Write data into the temp file
    FileStream out(DummyFile, kFile_CreateAlways, kFile_Write);
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    out.WriteInt32(3);
    // fill in to ensure buffered stream reach buffer size
    out.WriteByteCount(0, BufferedStream::BufferSize);
    const auto section1_start = out.GetPosition();
    out.WriteInt32(4);
    out.WriteInt32(5);
    out.WriteInt32(6);
    out.WriteInt32(7);
    const auto section1_end = out.GetPosition();
    out.WriteByteCount(0, BufferedStream::BufferSize);
    out.WriteInt32(8);
    out.WriteInt32(9);
    out.WriteInt32(10);
    out.WriteInt32(11);
    const auto section2_end = out.GetPosition();
    out.Close();

    //-------------------------------------------------------------------------
    // Read data back
    BufferedSectionStream in(DummyFile, section1_start, section1_end, kFile_Open, kFile_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_EQ(in.GetPosition(), 0);
    ASSERT_EQ(in.GetLength(), section1_end - section1_start);
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 6);
    ASSERT_EQ(in.ReadInt32(), 7);
    ASSERT_EQ(in.GetPosition(), section1_end - section1_start);
    ASSERT_TRUE(in.EOS());
    in.Close();

    // Test seeks
    BufferedSectionStream in2(DummyFile, section1_start, section2_end, kFile_Open, kFile_Read);
    ASSERT_TRUE(in2.CanRead());
    ASSERT_TRUE(in2.CanSeek());
    ASSERT_EQ(in2.GetPosition(), 0);
    ASSERT_EQ(in2.GetLength(), section2_end - section1_start);
    in2.Seek(4 * sizeof(int32_t) + BufferedStream::BufferSize, kSeekBegin);
    ASSERT_EQ(in2.ReadInt32(), 8);
    ASSERT_EQ(in2.ReadInt32(), 9);
    ASSERT_EQ(in2.ReadInt32(), 10);
    ASSERT_EQ(in2.ReadInt32(), 11);
    in2.Seek(0, kSeekBegin);
    ASSERT_EQ(in2.ReadInt32(), 4);
    ASSERT_EQ(in2.ReadInt32(), 5);
    ASSERT_EQ(in2.ReadInt32(), 6);
    ASSERT_EQ(in2.ReadInt32(), 7);
    ASSERT_EQ(in2.GetPosition(), 4 * sizeof(int32_t));
    in2.Seek(0, kSeekEnd);
    ASSERT_EQ(in2.GetPosition(), section2_end - section1_start);
    ASSERT_TRUE(in2.EOS());
    in2.Close();

    File::DeleteFile(DummyFile);
}

#endif // AGS_PLATFORM_TEST_FILE_IO


//-------------------------------------------------------------------------
// AlignedStream tests, for backward compatible data serialization
//-------------------------------------------------------------------------

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

TTrickyAlignedData makeTTricky()
{
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
#endif
    }
    return tricky_data_out;
}

// this builds the array that is in AlignedStream membuf for testing
// has to be compiled to Win32, x86, in a Windows machine, with MSVC
#if 0
#include <fstream>
void CreateWin32LegacyData() {
    // Storage buffer
    std::vector<char> membuf;

    //-------------------------------------------------------------------------
   // Write data
    std::unique_ptr<Stream> out(
        new MemoryStream(membuf, kStream_Write));

    TTrickyAlignedData tricky_data_out = makeTTricky();

#if defined (TEST_BIGENDIAN)
    out->Write(&bigend_data, sizeof(TTrickyAlignedData));
#else
    out->Write(&tricky_data_out, sizeof(TTrickyAlignedData));
#endif

    out->WriteInt32(20);

    out.reset();

    std::ofstream f;
    f.open("tricky_arr.csv");
    for (int i = 0; i < membuf.size(); i++) 
    {
        f << +membuf[i] << ", ";
    }
    f.close();

    ASSERT_TRUE(true);
}
#endif

TEST(Stream, AlignedStream) {
    // Storage buffer
    std::vector<uint8_t> membuf = { 11, 0xAA, 0xAA, 0xAA, 12, 0, 0, 0, 13, 0, 0, 0, 14, 0, 15, 0, 16, 0, 0xAA, 0xAA, 17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xAA, 0xAA, 0xAA, 18, 0, 0, 0, 19, 0, 0, 0, 20, 0, 0, 0, 21, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 22, 23, 24, 0xAA, 25, 0, 26, 0, 0, 0, 27, 0, 28, 0, 29, 0, 0, 0, 30, 0, 0, 0, 0, 0, 0, 0, 31, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 32, 0, 0, 0, 0, 0, 0, 0, 33, 0, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 34, 0, 0, 0, 0, 0, 0, 0, 35, 0, 36, 0, 0xAA, 0xAA, 0xAA, 0xAA, 37, 0, 0, 0, 0, 0, 0, 0, 38, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 20, 0, 0, 0 };

    TTrickyAlignedData tricky_data_out = makeTTricky();

    //-------------------------------------------------------------------------
    // Read data back
    std::unique_ptr<Stream> in(
        new VectorStream(membuf));

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
