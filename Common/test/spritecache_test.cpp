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
#include "ac/spritecache.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

using namespace AGS::Common;

const size_t SPRITE_FILE_HEADER_SZ = 28;
const size_t EMPTY_DAT_SZ = 2;
const size_t SPRITE_DAT_HEADER_SZ = 12;

static void FillSpriteCache(SpriteCache *sc)
{
	sc->SetSprite(1, std::make_unique<Bitmap>(1, 1, 32));
	sc->SetSprite(2, std::make_unique<Bitmap>(2, 2, 32));
	sc->SetEmptySprite(3, true);
	sc->SetEmptySprite(4, false);
	// skip few slots
	sc->SetSprite(10, std::make_unique<Bitmap>(10, 10, 32));
}

static void FillSpriteCache2(SpriteCache* sc)
{
	sc->SetSprite(3, std::make_unique<Bitmap>(3, 3, 32));
	sc->SetSprite(4, std::make_unique<Bitmap>(4, 4, 32));
	sc->SetSprite(8, std::make_unique<Bitmap>(8, 8, 32));
}

TEST(SpriteCache, SpriteCounts) {
	std::vector<SpriteInfo> spr_infos;
	auto sc = std::make_unique<SpriteCache>(spr_infos, SpriteCache::Callbacks());
	FillSpriteCache(sc.get());
	ASSERT_EQ(sc->GetSpriteSlotCount(), 11);
	ASSERT_EQ(sc->GetTopmostSprite(), 10);
	// Empty sprites are also counted as occupied slots, mark them as "assets" to be certain
	sc->SetEmptySprite(15, true);
	sc->SetEmptySprite(20, true);
	ASSERT_EQ(sc->GetSpriteSlotCount(), 21);
	ASSERT_EQ(sc->GetTopmostSprite(), 20);
	sc->DeleteSprite(20);
	// NOTE: sprite slot count is uncertain here, it may be less, or kept at previous max size,
	// depending on how the SpriteCache's allocation policy works.
	ASSERT_EQ(sc->GetTopmostSprite(), 15);
	sc->RemoveSprite(15);
	ASSERT_EQ(sc->GetTopmostSprite(), 10);
}

TEST(SpriteCache, SaveToFileOnlyMemoryBitmaps) {
	std::vector<SpriteInfo> spr_infos;
	auto sc = std::make_unique<SpriteCache>(spr_infos, SpriteCache::Callbacks());
	FillSpriteCache(sc.get());

	std::vector<uint8_t> storage;
	SpriteFileIndex index;
	HError err = sc->SaveToFile(std::make_unique<Stream>(std::make_unique<VectorStream>(storage, kStream_Write)),
		0, kSprCompress_None, index);

	ASSERT_TRUE(err);
	ASSERT_EQ(index.GetCount(), 11); // number of index entries
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

TEST(SpriteCache, SaveToFileAndInitBack) {
	std::vector<uint8_t> storage;

	{
		std::vector<SpriteInfo> spr_infos_temp;
		auto sc_temp = std::make_unique<SpriteCache>(spr_infos_temp, SpriteCache::Callbacks());
		FillSpriteCache(sc_temp.get());
		SpriteFileIndex index;
		sc_temp->SaveToFile(std::make_unique<Stream>(std::make_unique<VectorStream>(storage, kStream_Write)),
			0, kSprCompress_None, index);
	}

	std::vector<SpriteInfo> spr_infos;
	auto sc = std::make_unique<SpriteCache>(spr_infos, SpriteCache::Callbacks());
	HError err = sc->InitFile(std::make_unique<Stream>(std::make_unique<VectorStream>(storage)), nullptr);

	ASSERT_TRUE(err);
	ASSERT_EQ(sc->GetSpriteSlotCount(), 11);
	ASSERT_EQ(sc->GetTopmostSprite(), 10);
	ASSERT_EQ(spr_infos.size(), 11);
	ASSERT_EQ(spr_infos[0].Width, 0);
	ASSERT_EQ(spr_infos[1].Width, 1);
	ASSERT_EQ(spr_infos[2].Width, 2);
	ASSERT_EQ(spr_infos[3].Width, 0);
	ASSERT_EQ(spr_infos[4].Width, 0);
	ASSERT_EQ(spr_infos[10].Width, 10);
	ASSERT_EQ(spr_infos[0].Height, 0);
	ASSERT_EQ(spr_infos[1].Height, 1);
	ASSERT_EQ(spr_infos[2].Height, 2);
	ASSERT_EQ(spr_infos[3].Height, 0);
	ASSERT_EQ(spr_infos[4].Height, 0);
	ASSERT_EQ(spr_infos[10].Height, 10);
}

TEST(SpriteCache, SaveToFileUsingInputFile) {
	std::vector<uint8_t> storage1;
	std::vector<uint8_t> storage2;

	{
		std::vector<SpriteInfo> spr_infos_temp;
		auto sc_temp = std::make_unique<SpriteCache>(spr_infos_temp, SpriteCache::Callbacks());
		FillSpriteCache(sc_temp.get());
		SpriteFileIndex index;
		sc_temp->SaveToFile(std::make_unique<Stream>(std::make_unique<VectorStream>(storage1, kStream_Write)),
			0, kSprCompress_None, index);
	}
	
	std::vector<SpriteInfo> spr_infos;
	auto sc = std::make_unique<SpriteCache>(spr_infos, SpriteCache::Callbacks());
	sc->InitFile(std::make_unique<Stream>(std::make_unique<VectorStream>(storage1)), nullptr);
	FillSpriteCache2(sc.get());

	SpriteFileIndex index;
	HError err = sc->SaveToFile(std::make_unique<Stream>(std::make_unique<VectorStream>(storage2, kStream_Write)),
		0, kSprCompress_None, index);

	ASSERT_TRUE(err);
	ASSERT_EQ(index.GetCount(), 11); // number of index entries
	ASSERT_EQ(index.GetLastSlot(), 10);
	ASSERT_EQ(index.Widths.size(), 11);
	ASSERT_EQ(index.Heights.size(), 11);
	ASSERT_EQ(index.Offsets.size(), 11);
	ASSERT_EQ(index.Widths[0], 0);
	ASSERT_EQ(index.Widths[1], 1);
	ASSERT_EQ(index.Widths[2], 2);
	ASSERT_EQ(index.Widths[3], 3);
	ASSERT_EQ(index.Widths[4], 4);
	ASSERT_EQ(index.Widths[8], 8);
	ASSERT_EQ(index.Widths[10], 10);
	ASSERT_EQ(index.Heights[0], 0);
	ASSERT_EQ(index.Heights[1], 1);
	ASSERT_EQ(index.Heights[2], 2);
	ASSERT_EQ(index.Heights[3], 3);
	ASSERT_EQ(index.Heights[4], 4);
	ASSERT_EQ(index.Heights[8], 8);
	ASSERT_EQ(index.Heights[10], 10);
	soff_t off = SPRITE_FILE_HEADER_SZ;
	ASSERT_EQ(index.Offsets[0], off); off += EMPTY_DAT_SZ;
	ASSERT_EQ(index.Offsets[1], off); off += SPRITE_DAT_HEADER_SZ + (1 * 1 * 4);
	ASSERT_EQ(index.Offsets[2], off); off += SPRITE_DAT_HEADER_SZ + (2 * 2 * 4);
	ASSERT_EQ(index.Offsets[3], off); off += SPRITE_DAT_HEADER_SZ + (3 * 3 * 4);
	ASSERT_EQ(index.Offsets[4], off); off += SPRITE_DAT_HEADER_SZ + (4 * 4 * 4);
	for (size_t i = 5; i < 8; ++i) off += EMPTY_DAT_SZ;
	ASSERT_EQ(index.Offsets[8], off); off += SPRITE_DAT_HEADER_SZ + (8 * 8 * 4);
	ASSERT_EQ(index.Offsets[9], off); off += EMPTY_DAT_SZ;
	ASSERT_EQ(index.Offsets[10], off); off += SPRITE_DAT_HEADER_SZ + (10 * 10 * 4);
}
