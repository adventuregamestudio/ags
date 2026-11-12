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
#include <array>
#include "gtest/gtest.h"
#include "util/deflatestream.h"
#include "util/lzw.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

using namespace AGS::Common;

void WriteDeflatedData(const std::vector<uint8_t> *src_buf, size_t src_size_bytes,
    std::vector<uint8_t> &zip_buf, size_t &deflated_bytes)
{
    std::vector<uint8_t> src_buf_temp;
    if (!src_buf)
    {
        auto fill_src = std::make_unique<VectorStream>(src_buf_temp, kStream_Write);
        for (size_t i = 0; i < src_size_bytes; ++i)
            fill_src->WriteByte(0);
        src_buf = &src_buf_temp;
    }

    auto mem_src = std::make_unique<VectorStream>(*src_buf);
    auto deflate_s = std::make_unique<DeflateStream>(
        std::make_unique<VectorStream>(zip_buf, kStream_Write), kStream_Write);
    deflated_bytes = static_cast<size_t>(CopyStream(mem_src.get(), deflate_s.get()));
    // TODO: this is really bad, stream should auto-finalize on destruct, investigate and fix
    deflate_s->Finalize(); // explicitly finalize
}

void TestDeflateStream(const std::vector<uint8_t> src_buf)
{
    std::vector<uint8_t> zip_buf;
    std::vector<uint8_t> unzip_buf;

    size_t deflated_bytes = 0u;
    WriteDeflatedData(&src_buf, 0, zip_buf, deflated_bytes);

    ASSERT_EQ(deflated_bytes, src_buf.size());
    ASSERT_TRUE(zip_buf.size() > 0u);

    auto inflate_s = std::make_unique<DeflateStream>(
        std::make_unique<VectorStream>(zip_buf), kStream_Read);
    auto mem_unzip = std::make_unique<VectorStream>(unzip_buf, kStream_Write);
    const soff_t inflated_bytes = CopyStream(inflate_s.get(), mem_unzip.get());
    inflate_s = nullptr;
    mem_unzip = nullptr;

    ASSERT_EQ(inflated_bytes, src_buf.size());
    ASSERT_TRUE(std::equal(src_buf.begin(), src_buf.end(), unzip_buf.begin()));
}

void TestDeflateStream_OneValue(size_t buffer_size)
{
    std::vector<uint8_t> src_buf;
    const size_t InputBufferLength = buffer_size;
    src_buf.resize(InputBufferLength);

    std::fill(src_buf.begin(), src_buf.end(), 5u);

    TestDeflateStream(src_buf);
}

void TestDeflateStream_RepeatSeqs(size_t buffer_size)
{
    std::vector<uint8_t> src_buf;
    const size_t InputBufferLength = buffer_size;
    src_buf.resize(InputBufferLength);

    std::array<std::array<uint8_t, 12>, 12> seqs = {{
        { 1,2,3,4,5,6,7,8,9,10,11,12 },
        { 12,11,10,9,8,7,6,5,4,3,2,1 },
        { 1,2,3,4 },
        { 4,2,3,1 },
        { 1,8,1,7,1,6,1,5 },
        { 8,7,8,6,8,5,8,4 },
        { 4,4,3,3,3,2,2,2,2 },
        { 12,12,11,11,11,10,10,10 },
        { 12,1,11,2,10,3,9,4,8,5 },
        { 1,1,1,10,10,10,2,2,9,9,3,8 },
        { 4,4,4,4,3,3,3,2,2,1 },
        { 1,7,4,2,9,10,5,12,3,11,6 },
        }};

    // Always use same seed in this test to make it determenistic
    std::srand(UINT32_MAX);
    for (size_t at = 0; at < InputBufferLength;)
    {
        std::array<uint8_t, 12> &seq = seqs[std::rand() % 12];
        for (size_t ichar = 0; at < InputBufferLength && ichar < seq.size() && seq[ichar] != 0; ++ichar, ++at)
            src_buf[at] = seq[ichar];
    }

    TestDeflateStream(src_buf);
}

void TestDeflateStream_SemiUniqueSeq(size_t buffer_size)
{
    std::vector<uint8_t> src_buf;
    const size_t InputBufferLength = buffer_size;
    src_buf.resize(InputBufferLength);

    // Fill the buffer with numbers using some rule which prevents equal sequences
    uint8_t k = 0, l = 0, m = 0, n = 0;
    for (size_t seq = 0; seq < InputBufferLength % 4; ++seq)
    {
        size_t i = seq * 4;
        src_buf[i + 0] = k;
        src_buf[i + 1] = l;
        src_buf[i + 2] = m;
        src_buf[i + 3] = n;

        k++;
        if (k == 0)
        {
            l += 2;
            if (l < 2)
            {
                m += 3;
                if (m < 3)
                    n += 4;
            }
        }
    }

    TestDeflateStream(src_buf);
}

TEST(DeflateStream, DeflateStream_OneValue_InputEqBuffer) {
    // Input is equal to the DeflateStream's internal buffer
    TestDeflateStream_OneValue(DeflateStream::BufferSize);
}

TEST(DeflateStream, DeflateStream_OneValue_InputLtBuffer) {
    // Input is smaller than the DeflateStream's internal buffer
    TestDeflateStream_OneValue(DeflateStream::BufferSize / 4);
}

TEST(DeflateStream, DeflateStream_OneValue_InputGtBuffer) {
    // Input is larger than the DeflateStream's internal buffer
    TestDeflateStream_OneValue(DeflateStream::BufferSize * 2 + DeflateStream::BufferSize / 27);
}

TEST(DeflateStream, DeflateStream_RepeatSeqs_InputEqBuffer) {
    // Input is equal to the DeflateStream's internal buffer
    TestDeflateStream_RepeatSeqs(DeflateStream::BufferSize);
}

TEST(DeflateStream, DeflateStream_RepeatSeqs_InputLtBuffer) {
    // Input is smaller than the DeflateStream's internal buffer
    TestDeflateStream_RepeatSeqs(DeflateStream::BufferSize / 4);
}

TEST(DeflateStream, DeflateStream_RepeatSeqs_InputGtBuffer) {
    // Input is larger than the DeflateStream's internal buffer
    TestDeflateStream_RepeatSeqs(DeflateStream::BufferSize * 2 + DeflateStream::BufferSize / 27);
}

TEST(DeflateStream, DeflateStream_SemiUnique_InputEqBuffer) {
    // Input is equal to the DeflateStream's internal buffer
    TestDeflateStream_SemiUniqueSeq(DeflateStream::BufferSize);
}

TEST(DeflateStream, DeflateStream_SemiUnique_InputLtBuffer) {
    // Input is smaller than the DeflateStream's internal buffer
    TestDeflateStream_SemiUniqueSeq(DeflateStream::BufferSize / 4);
}

TEST(DeflateStream, DeflateStream_SemiUnique_InputGtBuffer) {
    // Input is larger than the DeflateStream's internal buffer
    TestDeflateStream_SemiUniqueSeq(DeflateStream::BufferSize * 2 + DeflateStream::BufferSize / 27);
}

void TestDeflateStreamEOS(size_t data_size)
{
    std::vector<uint8_t> zip_buf;
    std::vector<uint8_t> unzip_buf;
    size_t deflated_bytes = 0u;
    WriteDeflatedData(nullptr, data_size, zip_buf, deflated_bytes);

    auto inflate_s = std::make_unique<DeflateStream>(
        std::make_unique<VectorStream>(zip_buf), kStream_Read);
    auto mem_unzip = std::make_unique<VectorStream>(unzip_buf, kStream_Write);

    size_t read_count = 0u;
    for (; !inflate_s->EOS(); inflate_s->ReadByte(), ++read_count);
    ASSERT_EQ(data_size, read_count);
}

TEST(DeflateStream, DeflateStream_EOS_HalfBuffer) {
    TestDeflateStreamEOS(DeflateStream::BufferSize / 2);
}

TEST(DeflateStream, DeflateStream_EOS_OneBuffer) {
    TestDeflateStreamEOS(DeflateStream::BufferSize);
}

TEST(DeflateStream, DeflateStream_EOS_TwoBuffers) {
    TestDeflateStreamEOS(DeflateStream::BufferSize * 2);
}

TEST(DeflateStream, DeflateStream_EOS_OddBufferSize) {
    TestDeflateStreamEOS(DeflateStream::BufferSize * 2 + DeflateStream::BufferSize / 27);
}

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
