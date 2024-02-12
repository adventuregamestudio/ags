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
#include <array>
#include <memory>
#include <vector>
#include "gtest/gtest.h"
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
    ASSERT_EQ(in2.Seek(4 * sizeof(int32_t) + fill_len, kSeekBegin), (4 * sizeof(int32_t) + fill_len));
    ASSERT_EQ(in2.ReadInt32(), 4);
    ASSERT_EQ(in2.Seek(2 * sizeof(int32_t), kSeekBegin), (2 * sizeof(int32_t)));
    ASSERT_EQ(in2.ReadInt32(), 2);
    ASSERT_EQ(in2.Seek(8 * sizeof(int32_t) + fill_len * 2, kSeekBegin), (8 * sizeof(int32_t) + fill_len * 2));
    ASSERT_EQ(in2.ReadInt32(), 8);
    ASSERT_EQ(in2.Seek(2 * sizeof(int32_t), kSeekCurrent), (8 * sizeof(int32_t) + fill_len * 2) + sizeof(int32_t) * 3);
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

TEST(Stream, DataStreamSection) {
    // Storage buffer
    std::vector<uint8_t> membuf;
    const size_t fill_len = 10;
    //-------------------------------------------------------------------------
    // Write data
    VectorStream out(membuf, kStream_Write);
    // We write and read int8s, because it's easier to test single bytes here
    for (size_t i = 0; i < fill_len; ++i)
        out.WriteInt8(static_cast<uint8_t>(i));
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back using SectionStreams
    VectorStream in(membuf, kStream_Read);
    DataStreamSection sect1(&in, 4, 6);
    ASSERT_TRUE(sect1.CanRead());
    ASSERT_TRUE(sect1.CanSeek());
    ASSERT_EQ(sect1.GetPosition(), 0);
    ASSERT_EQ(sect1.GetLength(), 6 - 4);
    // Make sure that the base stream is at the expected spot too
    ASSERT_EQ(in.GetPosition(), 4);
    int expect_value = 4;
    while (!sect1.EOS())
        ASSERT_EQ(sect1.ReadInt8(), expect_value++);
    sect1.Close();
    // Make sure that the base stream is still valid,
    // and at the expected spot too
    ASSERT_TRUE(in.IsValid());
    ASSERT_EQ(in.GetPosition(), 6);

    expect_value = 2;
    DataStreamSection sect2(&in, 2, 8);
    while (!sect2.EOS())
        ASSERT_EQ(sect2.ReadInt8(), expect_value++);
    sect2.Close();
    ASSERT_EQ(in.GetPosition(), 8);

    // Test seeks
    DataStreamSection sect_seek(&in, 2, 8);
    ASSERT_EQ(sect_seek.GetPosition(), 0);
    ASSERT_EQ(sect_seek.GetLength(), 8 - 2);
    ASSERT_EQ(in.GetPosition(), 2);
    ASSERT_EQ(sect_seek.Seek(4, kSeekBegin), 4);
    ASSERT_EQ(sect_seek.GetPosition(), 4);
    ASSERT_EQ(in.GetPosition(), 2 + 4);
    ASSERT_EQ(sect_seek.Seek(2, kSeekCurrent), 6);
    ASSERT_EQ(sect_seek.GetPosition(), 6);
    ASSERT_EQ(in.GetPosition(), 2 + 6);
    ASSERT_EQ(sect_seek.Seek(-4, kSeekCurrent), 2);
    ASSERT_EQ(sect_seek.GetPosition(), 2);
    ASSERT_EQ(in.GetPosition(), 2 + 2);
    ASSERT_EQ(sect_seek.Seek(-3, kSeekEnd), 3);
    ASSERT_EQ(sect_seek.GetPosition(), 3);
    ASSERT_EQ(in.GetPosition(), 2 + 3);
    sect_seek.Close();
    in.Close();
}

#if (AGS_PLATFORM_TEST_FILE_IO)

static const char *DummyFile = "dummy.dat";

class FileBasedTest : public ::testing::Test {
protected:
    void SetUp() override {
        File::DeleteFile(DummyFile);
    }

    void TearDown() override {
        File::DeleteFile(DummyFile);
    }
};

TEST_F(FileBasedTest, BufferedStreamRead) {
    //-------------------------------------------------------------------------
    // Write data into the temp file
    FileStream out(DummyFile, kFile_CreateAlways, kStream_Write);
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
    BufferedStream in(DummyFile, kFile_Open, kStream_Read);
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
    BufferedStream in2(DummyFile, kFile_Open, kStream_Read);
    ASSERT_TRUE(in2.CanRead());
    ASSERT_TRUE(in2.CanSeek());
    ASSERT_EQ(in2.GetLength(), file_len);
    ASSERT_EQ(in2.Seek(4 * sizeof(int32_t) + BufferedStream::BufferSize, kSeekBegin), (4 * sizeof(int32_t) + BufferedStream::BufferSize));
    ASSERT_EQ(in2.ReadInt32(), 4);
    ASSERT_EQ(in2.Seek(2 * sizeof(int32_t), kSeekBegin), (2 * sizeof(int32_t)));
    ASSERT_EQ(in2.ReadInt32(), 2);
    ASSERT_EQ(in2.Seek(8 * sizeof(int32_t) + BufferedStream::BufferSize * 2, kSeekBegin), (8 * sizeof(int32_t) + BufferedStream::BufferSize * 2));
    ASSERT_EQ(in2.ReadInt32(), 8);
    ASSERT_EQ(in2.Seek(2 * sizeof(int32_t), kSeekCurrent), (8 * sizeof(int32_t) + BufferedStream::BufferSize * 2) + sizeof(int32_t) * 3);
    ASSERT_EQ(in2.ReadInt32(), 11);
    ASSERT_EQ(in2.GetPosition(), file_len);
    ASSERT_TRUE(in2.EOS());
    in2.Close();

    File::DeleteFile(DummyFile);
}

TEST_F(FileBasedTest, BufferedStreamWrite1) {
    // Test case 1: simple straight writing, within max buffer size
    //-------------------------------------------------------------------------
    // Write data
    const soff_t file_len = sizeof(int32_t) * 10;
    BufferedStream out(DummyFile, kFile_CreateAlways, kStream_Write);
    ASSERT_TRUE(out.CanWrite());
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    out.WriteInt32(3);
    out.WriteInt32(4);
    out.WriteInt32(5);
    out.WriteInt32(6);
    out.WriteInt32(7);
    out.WriteInt32(8);
    out.WriteInt32(9);
    ASSERT_EQ(out.GetPosition(), file_len);
    ASSERT_EQ(out.GetLength(), file_len);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    FileStream in(DummyFile, kFile_Open, kStream_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_TRUE(in.CanSeek());
    ASSERT_EQ(in.GetLength(), file_len);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 3);
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 6);
    ASSERT_EQ(in.ReadInt32(), 7);
    ASSERT_EQ(in.ReadInt32(), 8);
    ASSERT_EQ(in.ReadInt32(), 9);
    ASSERT_EQ(in.GetPosition(), file_len);
    in.Close();

    File::DeleteFile(DummyFile);
}

TEST_F(FileBasedTest, BufferedStreamWrite2) {
    // Test case 2: simple straight writing, exceeding max buffer size
    //-------------------------------------------------------------------------
    // fill in to ensure buffered stream reach buffer size
    size_t fill_len = BufferedStream::BufferSize;
    //-------------------------------------------------------------------------
    // Write data
    const soff_t file_len = sizeof(int32_t) * 10 + fill_len;
    BufferedStream out(DummyFile, kFile_CreateAlways, kStream_Write);
    ASSERT_TRUE(out.CanWrite());
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    out.WriteInt32(3);
    out.WriteInt32(4);
    out.WriteInt32(5);
    out.WriteInt32(6);
    out.WriteByteCount(0, fill_len); // fill to force buffer flush
    out.WriteInt32(7);
    out.WriteInt32(8);
    out.WriteInt32(9);
    ASSERT_EQ(out.GetPosition(), file_len);
    ASSERT_EQ(out.GetLength(), file_len);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    FileStream in(DummyFile, kFile_Open, kStream_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_TRUE(in.CanSeek());
    ASSERT_EQ(in.GetLength(), file_len);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 3);
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 6);
    in.Seek(fill_len, kSeekCurrent);
    ASSERT_EQ(in.ReadInt32(), 7);
    ASSERT_EQ(in.ReadInt32(), 8);
    ASSERT_EQ(in.ReadInt32(), 9);
    ASSERT_EQ(in.GetPosition(), file_len);
    in.Close();

    File::DeleteFile(DummyFile);
}

TEST_F(FileBasedTest, BufferedStreamWrite3) {
    // Test case 3: seek within the max buffer size
    //-------------------------------------------------------------------------
    // Write data
    const soff_t file_len = sizeof(int32_t) * 10;
    BufferedStream out(DummyFile, kFile_CreateAlways, kStream_Write);
    ASSERT_TRUE(out.CanWrite());
    ASSERT_TRUE(out.CanSeek());
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    out.WriteInt32(3);
    out.WriteInt32(4);
    auto write_back_pos = out.GetPosition();
    out.WriteInt32(5);
    out.WriteInt32(6);
    out.WriteInt32(7);
    out.WriteInt32(8);
    auto end_pos = out.GetPosition();
    out.Seek(write_back_pos, kSeekBegin);
    out.WriteInt32(111);
    out.Seek(end_pos, kSeekBegin);
    out.WriteInt32(9);
    ASSERT_EQ(out.GetPosition(), file_len);
    ASSERT_EQ(out.GetLength(), file_len);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    FileStream in(DummyFile, kFile_Open, kStream_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_EQ(in.GetLength(), file_len);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 3);
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 111);
    ASSERT_EQ(in.ReadInt32(), 6);
    ASSERT_EQ(in.ReadInt32(), 7);
    ASSERT_EQ(in.ReadInt32(), 8);
    ASSERT_EQ(in.ReadInt32(), 9);
    ASSERT_EQ(in.GetPosition(), file_len);
    in.Close();

    File::DeleteFile(DummyFile);
}

TEST_F(FileBasedTest, BufferedStreamWrite4) {
    // Test case 4: seek outside the max buffer size
    //-------------------------------------------------------------------------
    // fill in to ensure buffered stream reach buffer size
    size_t fill_len = BufferedStream::BufferSize;
    //-------------------------------------------------------------------------
    // Write data
    const soff_t file_len = sizeof(int32_t) * 8 + fill_len;
    BufferedStream out(DummyFile, kFile_CreateAlways, kStream_Write);
    ASSERT_TRUE(out.CanWrite());
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    auto write_back_pos = out.GetPosition();
    out.WriteInt32(3);
    out.WriteInt32(4);
    out.WriteByteCount(0, fill_len); // fill to force buffer flush
    out.WriteInt32(5);
    out.WriteInt32(6);
    auto write_end_pos = out.GetPosition();
    out.Seek(write_back_pos, kSeekBegin);
    out.WriteInt32(111);
    out.Seek(write_end_pos, kSeekEnd);
    out.WriteInt32(222);
    ASSERT_EQ(out.GetPosition(), file_len);
    ASSERT_EQ(out.GetLength(), file_len);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    FileStream in(DummyFile, kFile_Open, kStream_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_TRUE(in.CanSeek());
    ASSERT_EQ(in.GetLength(), file_len);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 1);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 111);
    ASSERT_EQ(in.ReadInt32(), 4);
    in.Seek(fill_len, kSeekCurrent);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 6);
    ASSERT_EQ(in.ReadInt32(), 222);
    ASSERT_EQ(in.GetPosition(), file_len);
    in.Close();

    File::DeleteFile(DummyFile);
}

TEST_F(FileBasedTest, BufferedStreamWrite5) {
    // Test case 5: write provoking buffer flush, but seek within max buffer size
    //-------------------------------------------------------------------------
    // fill in to ensure buffered stream (almost) reach buffer size
    size_t fill_len = BufferedStream::BufferSize - sizeof(int32_t) * 4;
    //-------------------------------------------------------------------------
    // Write data
    const soff_t file_len = sizeof(int32_t) * 7 + fill_len;
    BufferedStream out(DummyFile, kFile_CreateAlways, kStream_Write);
    ASSERT_TRUE(out.CanWrite());
    out.WriteByteCount(0, fill_len); // fill to (nearly) force buffer flush
    out.WriteInt32(0);
    auto write_back_pos = out.GetPosition();
    out.WriteInt32(1);
    out.WriteInt32(2);
    out.WriteInt32(3); // buffer filled here
    out.WriteInt32(4); // buffer flushed here
    out.WriteInt32(5);
    auto write_end_pos = out.GetPosition();
    out.Seek(write_back_pos, kSeekBegin); // buffer reset at the new location
    out.WriteInt32(111);
    out.Seek(write_end_pos, kSeekBegin); // still within the max buffer size, no reset
    out.WriteInt32(222);
    ASSERT_EQ(out.GetPosition(), file_len);
    ASSERT_EQ(out.GetLength(), file_len);
    out.Close();
    //-------------------------------------------------------------------------
    // Read data back
    FileStream in(DummyFile, kFile_Open, kStream_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_TRUE(in.CanSeek());
    ASSERT_EQ(in.GetLength(), file_len);
    in.Seek(fill_len, kSeekCurrent);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.ReadInt32(), 111);
    ASSERT_EQ(in.ReadInt32(), 2);
    ASSERT_EQ(in.ReadInt32(), 3);
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 222);
    ASSERT_EQ(in.GetPosition(), file_len);
    in.Close();

    File::DeleteFile(DummyFile);
}

TEST_F(FileBasedTest, BufferedSectionStream) {
    //-------------------------------------------------------------------------
    // Write data into the temp file
    FileStream out(DummyFile, kFile_CreateAlways, kStream_Write);
    out.WriteInt32(0);
    out.WriteInt32(1);
    out.WriteInt32(2);
    out.WriteInt32(3);
    // fill in to ensure buffered stream reach buffer size
    out.WriteByteCount(0xFF, BufferedStream::BufferSize);
    const auto section1_start = out.GetPosition();
    out.WriteInt32(4);
    out.WriteInt32(5);
    out.WriteInt32(6);
    out.WriteInt32(7);
    const auto section1_end = out.GetPosition();
    out.WriteByteCount(0xFF, BufferedStream::BufferSize);
    out.WriteInt32(8);
    out.WriteInt32(9);
    out.WriteInt32(10);
    out.WriteInt32(11);
    const auto section2_end = out.GetPosition();
    out.Close();

    //-------------------------------------------------------------------------
    // Read data back from section 1 and test read limits
    BufferedSectionStream in(DummyFile, section1_start, section1_end, kFile_Open, kStream_Read);
    ASSERT_TRUE(in.CanRead());
    ASSERT_EQ(in.GetPosition(), 0);
    ASSERT_EQ(in.GetLength(), section1_end - section1_start);
    ASSERT_EQ(in.ReadInt32(), 4);
    ASSERT_EQ(in.ReadInt32(), 5);
    ASSERT_EQ(in.ReadInt32(), 6);
    ASSERT_EQ(in.ReadInt32(), 7);
    ASSERT_EQ(in.GetPosition(), section1_end - section1_start);
    ASSERT_TRUE(in.EOS());
    // reading past section end - results in no data
    char temp[10];
    ASSERT_EQ(in.ReadByte(), -1);
    ASSERT_EQ(in.ReadInt32(), 0);
    ASSERT_EQ(in.Read(temp, sizeof(temp)), 0); // read 0 bytes
    // must still be at section end
    ASSERT_EQ(in.GetPosition(), section1_end - section1_start);
    ASSERT_TRUE(in.EOS());
    in.Close();

    // Test limits - reading large chunks: optimized by reading directly
    // into the provided user's buffer, without use of internal buffer
    BufferedSectionStream in3(DummyFile, section1_start, section1_end, kFile_Open, kStream_Read);
    const size_t try_read = 4 * sizeof(int32_t) + BufferedStream::BufferSize;
    const size_t must_read = 4 * sizeof(int32_t);
    char buf[try_read];
    // must not read past the end
    ASSERT_EQ(in3.Read(buf, try_read), must_read);
    // must still be at section end
    ASSERT_EQ(in3.GetPosition(), section1_end - section1_start);
    ASSERT_TRUE(in.EOS());
    in3.Close();

    // Test seeks limited to section 1
    BufferedSectionStream in2(DummyFile, section1_start, section2_end, kFile_Open, kStream_Read);
    ASSERT_TRUE(in2.CanRead());
    ASSERT_TRUE(in2.CanSeek());
    ASSERT_EQ(in2.GetPosition(), 0);
    ASSERT_EQ(in2.GetLength(), section2_end - section1_start);
    ASSERT_EQ(in2.Seek(4 * sizeof(int32_t) + BufferedStream::BufferSize, kSeekBegin), (4 * sizeof(int32_t) + BufferedStream::BufferSize));
    ASSERT_EQ(in2.ReadInt32(), 8);
    ASSERT_EQ(in2.ReadInt32(), 9);
    ASSERT_EQ(in2.ReadInt32(), 10);
    ASSERT_EQ(in2.ReadInt32(), 11);
    ASSERT_EQ(in2.Seek(0, kSeekBegin), 0);
    ASSERT_EQ(in2.ReadInt32(), 4);
    ASSERT_EQ(in2.ReadInt32(), 5);
    ASSERT_EQ(in2.ReadInt32(), 6);
    ASSERT_EQ(in2.ReadInt32(), 7);
    ASSERT_EQ(in2.GetPosition(), 4 * sizeof(int32_t));
    ASSERT_EQ(in2.Seek(0, kSeekEnd), section2_end - section1_start);
    ASSERT_EQ(in2.GetPosition(), section2_end - section1_start);
    ASSERT_TRUE(in2.EOS());
    in2.Close();

    File::DeleteFile(DummyFile);
}

#endif // AGS_PLATFORM_TEST_FILE_IO
