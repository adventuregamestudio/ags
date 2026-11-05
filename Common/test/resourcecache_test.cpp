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
#include "util/resourcecache.h"

using namespace AGS::Common;

// Test cache of ints, where the value is also the item's size
class TestCache : public ResourceCache<int, int>
{
public:
    TestCache() : ResourceCache<int, int>(1000) {}

private:
    size_t CalcSize(const int &item) override { return item; }
};

TEST(ResourceCache, DisposeFreeItemsKeepsLockedItems)
{
    TestCache cache;
    cache.Put(1, 10);
    cache.Put(2, 20);
    cache.Put(3, 30);
    cache.Put(4, 40, TestCache::kCacheItem_Locked);
    cache.Put(5, 50, TestCache::kCacheItem_Locked);
    ASSERT_EQ(150u, cache.GetCacheSize());
    ASSERT_EQ(90u, cache.GetLockedSize());

    cache.DisposeFreeItems();
    EXPECT_FALSE(cache.Exists(1));
    EXPECT_FALSE(cache.Exists(2));
    EXPECT_FALSE(cache.Exists(3));
    EXPECT_TRUE(cache.Exists(4));
    EXPECT_TRUE(cache.Exists(5));
    EXPECT_EQ(90u, cache.GetCacheSize());
    EXPECT_EQ(90u, cache.GetLockedSize());

    // the cache goes on working: items load, lock and release again
    cache.Put(6, 60);
    cache.Lock(6);
    cache.Release(4);
    EXPECT_EQ(150u, cache.GetCacheSize());
    EXPECT_EQ(110u, cache.GetLockedSize());
    cache.DisposeFreeItems();
    EXPECT_FALSE(cache.Exists(4));
    EXPECT_TRUE(cache.Exists(5));
    EXPECT_TRUE(cache.Exists(6));
    EXPECT_EQ(110u, cache.GetCacheSize());
}

TEST(ResourceCache, DisposeFreeItemsWithoutLockedItems)
{
    TestCache cache;
    cache.Put(1, 10);
    cache.Put(2, 20);
    cache.DisposeFreeItems();
    EXPECT_FALSE(cache.Exists(1));
    EXPECT_FALSE(cache.Exists(2));
    EXPECT_EQ(0u, cache.GetCacheSize());
}
