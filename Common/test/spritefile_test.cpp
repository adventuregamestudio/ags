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
#include "gtest/gtest.h"
#include "ac/gamestructdefines.h"
#include "ac/spritefile.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

using namespace AGS::Common;

const size_t SPRITE_FILE_HEADER_SZ = 28;
const size_t EMPTY_DAT_SZ = 2;
const size_t SPRITE_DAT_HEADER_SZ = 12;

static void WriteSpriteFile(SpriteFileWriter* sfw)
{
	sfw->Begin(0, kSprCompress_None);
	sfw->WriteEmptySlot();
	sfw->WriteBitmap(PixelBuffer(1, 1, kPxFmt_A8R8G8B8));
	sfw->WriteBitmap(PixelBuffer(2, 2, kPxFmt_A8R8G8B8));
	sfw->WriteEmptySlot();//3
	sfw->WriteEmptySlot();//4
	sfw->WriteEmptySlot();//5
	sfw->WriteEmptySlot();//6
	sfw->WriteEmptySlot();//7
	sfw->WriteEmptySlot();//8
	sfw->WriteEmptySlot();//9
	sfw->WriteBitmap(PixelBuffer(10, 10, kPxFmt_A8R8G8B8));
	sfw->Finalize();
}

TEST(SpriteFileWriter, WriteAndIndex) {
	SpriteFileIndex index;
	{
		std::vector<uint8_t> storage;
		auto sfw = std::make_unique<SpriteFileWriter>(
			std::make_unique<Stream>(std::make_unique<VectorStream>(storage, kStream_Write)));
		WriteSpriteFile(sfw.get());
		index = sfw->GetIndex();
	}
	
	ASSERT_EQ(index.GetCount(), 11);
	ASSERT_EQ(index.GetLastSlot(), 10);
	// FIXME: these should be hidden behind the getters,
	// but then we'd need factory methods for SpriteIndex struct
	ASSERT_EQ(index.Widths.size(), 11);
	ASSERT_EQ(index.Heights.size(), 11);
	ASSERT_EQ(index.Offsets.size(), 11);
	ASSERT_EQ(index.Widths[0], 0);
	ASSERT_EQ(index.Widths[1], 1);
	ASSERT_EQ(index.Widths[2], 2);
	ASSERT_EQ(index.Widths[3], 0);
	ASSERT_EQ(index.Widths[4], 0);
	ASSERT_EQ(index.Widths[10], 10);
	ASSERT_EQ(index.Heights[0], 0);
	ASSERT_EQ(index.Heights[1], 1);
	ASSERT_EQ(index.Heights[2], 2);
	ASSERT_EQ(index.Heights[3], 0);
	ASSERT_EQ(index.Heights[4], 0);
	ASSERT_EQ(index.Heights[10], 10);
	soff_t off = SPRITE_FILE_HEADER_SZ;
	ASSERT_EQ(index.Offsets[0], off); off += EMPTY_DAT_SZ;
	ASSERT_EQ(index.Offsets[1], off); off += SPRITE_DAT_HEADER_SZ + (1 * 1 * 4);
	ASSERT_EQ(index.Offsets[2], off); off += SPRITE_DAT_HEADER_SZ + (2 * 2 * 4);
	ASSERT_EQ(index.Offsets[3], off); off += EMPTY_DAT_SZ;
	ASSERT_EQ(index.Offsets[4], off); off += EMPTY_DAT_SZ;
	for (size_t i = 5; i < 10; ++i) off += EMPTY_DAT_SZ;
	ASSERT_EQ(index.Offsets[10], off); off += SPRITE_DAT_HEADER_SZ + (10 * 10 * 4);
}

TEST(SpriteFileWriter, WriteAndReadMetrics) {
	std::vector<uint8_t> storage;
	{
		auto sfw = std::make_unique<SpriteFileWriter>(
			std::make_unique<Stream>(std::make_unique<VectorStream>(storage, kStream_Write)));
		WriteSpriteFile(sfw.get());
	}

	std::vector<Size> metrics;
	auto sf = std::make_unique<SpriteFile>();
	HError err = sf->OpenFile(std::make_unique<Stream>(std::make_unique<VectorStream>(storage)), nullptr, metrics);

	ASSERT_TRUE(err);
	ASSERT_EQ(sf->GetSpriteCount(), 3); // valid count
	ASSERT_EQ(sf->GetTopmostSprite(), 10);
	ASSERT_EQ(metrics.size(), 11);
	ASSERT_EQ(metrics[0].Width, 0);
	ASSERT_EQ(metrics[1].Width, 1);
	ASSERT_EQ(metrics[2].Width, 2);
	ASSERT_EQ(metrics[3].Width, 0);
	ASSERT_EQ(metrics[4].Width, 0);
	ASSERT_EQ(metrics[10].Width, 10);
	ASSERT_EQ(metrics[0].Height, 0);
	ASSERT_EQ(metrics[1].Height, 1);
	ASSERT_EQ(metrics[2].Height, 2);
	ASSERT_EQ(metrics[3].Height, 0);
	ASSERT_EQ(metrics[4].Height, 0);
	ASSERT_EQ(metrics[10].Height, 10);
}
