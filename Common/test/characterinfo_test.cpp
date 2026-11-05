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
#include "ac/characterinfo.h"
#include "util/memory_compat.h"
#include "util/memorystream.h"

using namespace AGS::Common;

static std::vector<uint8_t> SaveCharacter(const CharacterInfo &chinfo)
{
    std::vector<uint8_t> membuf;
    Stream out(std::make_unique<VectorStream>(membuf, kStream_Write));
    chinfo.WriteToSavegame(&out);
    return membuf;
}

TEST(CharacterInfo, SavedInventoryDoesNotDependOnInsertionOrder)
{
    CharacterInfo ch_a, ch_b, ch_c;
    ch_a.set_item_quantity(1, 10);
    ch_a.set_item_quantity(5, 20);
    ch_a.set_item_quantity(9, 30);
    ch_b.set_item_quantity(9, 30);
    ch_b.set_item_quantity(5, 20);
    ch_b.set_item_quantity(1, 10);
    ch_c.set_item_quantity(5, 20);
    ch_c.set_item_quantity(9, 30);
    ch_c.set_item_quantity(1, 10);

    const std::vector<uint8_t> bytes_a = SaveCharacter(ch_a);
    EXPECT_EQ(bytes_a, SaveCharacter(ch_b));
    EXPECT_EQ(bytes_a, SaveCharacter(ch_c));
}

TEST(CharacterInfo, SavedInventoryIsInItemOrder)
{
    CharacterInfo with_items, without_items;
    with_items.set_item_quantity(7, 70);
    with_items.set_item_quantity(2, 20);
    with_items.set_item_quantity(4, 40);

    // the items are the only difference, and they come after the item count
    const std::vector<uint8_t> bytes_with = SaveCharacter(with_items);
    const std::vector<uint8_t> bytes_without = SaveCharacter(without_items);
    ASSERT_EQ(bytes_without.size() + 3 * 8, bytes_with.size());
    size_t at = 0;
    while (at < bytes_without.size() && bytes_with[at] == bytes_without[at])
        ++at;
    // the first difference is the item count (3 instead of 0), then the (ID, quantity) pairs
    ASSERT_LE(at + 4 + 3 * 8, bytes_with.size());
    const int32_t expected[] = { 2, 20, 4, 40, 7, 70 };
    for (size_t i = 0; i < 6; ++i)
    {
        const uint8_t *p = &bytes_with[at + 4 + i * 4];
        const int32_t value = static_cast<int32_t>(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24));
        EXPECT_EQ(expected[i], value);
    }
}
