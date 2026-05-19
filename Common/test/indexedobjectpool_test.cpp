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
#include <memory>
#include <vector>
#include "gtest/gtest.h"
#include "util/indexedobjectpool.h"


TEST(IndexedObjectPool, BasicTest) {
    IndexedObjectPool<int, int, -1> pool;

    // Initial state
    ASSERT_EQ(pool.GetCount(), 0u);
    ASSERT_EQ(pool.size(), 0u);

    // Adding new elements
    const int index0 = pool.Add(0);
    const int index1 = pool.Add(1);
    const int index2 = pool.Add(2);
    ASSERT_EQ(pool.GetCount(), 3u); // only used elements
    ASSERT_EQ(pool.size(), 3u); // all elements
    ASSERT_EQ(index0, 0);
    ASSERT_EQ(index1, 1);
    ASSERT_EQ(index2, 2);
    ASSERT_EQ(pool.IsFree(0), false);
    ASSERT_EQ(pool.IsFree(1), false);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsInUse(0), true);
    ASSERT_EQ(pool.IsInUse(1), true);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool[0], 0);
    ASSERT_EQ(pool[1], 1);
    ASSERT_EQ(pool[2], 2);

    // Freeing some elements
    pool.Free(1);
    pool.Free(2);
    ASSERT_EQ(pool.GetCount(), 1u); // only used elements
    ASSERT_EQ(pool.size(), 3u); // all elements
    ASSERT_EQ(pool.IsFree(0), false);
    ASSERT_EQ(pool.IsFree(1), true);
    ASSERT_EQ(pool.IsFree(2), true);
    ASSERT_EQ(pool.IsInUse(0), true);
    ASSERT_EQ(pool.IsInUse(1), false);
    ASSERT_EQ(pool.IsInUse(2), false);

    // Adding more elements, some go onto the previously freed slots,
    // chosen at the *reverse order of freeing* them, not order of index
    // (that's how the free index queue works internally).
    const int index3 = pool.Add(3);
    const int index4 = pool.Add(4);
    const int index5 = pool.Add(5);
    ASSERT_EQ(pool.GetCount(), 4u); // only used elements
    ASSERT_EQ(pool.size(), 4u); // all elements
    ASSERT_EQ(index3, 2);
    ASSERT_EQ(index4, 1);
    ASSERT_EQ(index5, 3);
    ASSERT_EQ(pool.IsFree(0), false);
    ASSERT_EQ(pool.IsFree(1), false);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(3), false);
    ASSERT_EQ(pool.IsInUse(0), true);
    ASSERT_EQ(pool.IsInUse(1), true);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool[0], 0);
    ASSERT_EQ(pool[1], 4);
    ASSERT_EQ(pool[2], 3);
    ASSERT_EQ(pool[3], 5);

    // Test Set
    pool.Free(0);
    pool.Free(1);
    pool.Free(2);
    pool.Free(3);
    pool.Set(6, 2);
    pool.Set(7, 0);
    pool.Set(8, 5); // expands to element 5
    ASSERT_EQ(pool.GetCount(), 3u); // only used elements
    ASSERT_EQ(pool.size(), 6u); // all elements
    ASSERT_EQ(pool.IsFree(0), false);
    ASSERT_EQ(pool.IsFree(1), true);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(3), true);
    ASSERT_EQ(pool.IsFree(4), true);
    ASSERT_EQ(pool.IsFree(5), false);
    ASSERT_EQ(pool.IsInUse(0), true);
    ASSERT_EQ(pool.IsInUse(1), false);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(3), false);
    ASSERT_EQ(pool.IsInUse(4), false);
    ASSERT_EQ(pool.IsInUse(5), true);
    // Free have reset elements to the type's default values
    {
        const auto &cpool = pool; // force const container, prevent STL assertions
        ASSERT_EQ(cpool[0], 7);
        ASSERT_EQ(cpool[1], 0);
        ASSERT_EQ(cpool[2], 6);
        ASSERT_EQ(cpool[3], 0);
        ASSERT_EQ(cpool[4], 0);
        ASSERT_EQ(cpool[5], 8);
    }

    // Cleared state
    pool.Clear();
    ASSERT_EQ(pool.GetCount(), 0u);
    ASSERT_EQ(pool.size(), 0u);
}

TEST(IndexedObjectPool, InvalidIndexIsZero) {
    IndexedObjectPool<int, int, 0> pool;

    // Initial state
    ASSERT_EQ(pool.GetCount(), 0u);
    ASSERT_EQ(pool.size(), 1u); // index 0 is persistent (internal logic)

    // 0 is a invalid index, therefore first index must be 1
    const int index = pool.Add(1);
    ASSERT_EQ(pool.GetCount(), 1u); // counts only valid elements in use
    ASSERT_EQ(pool.size(), 2u); // index 0 is persistent (internal logic), and new index 1
    ASSERT_EQ(index, 1);
    ASSERT_EQ(pool.IsFree(0), true);
    ASSERT_EQ(pool.IsFree(1), false);
    ASSERT_EQ(pool.IsInUse(0), false);
    ASSERT_EQ(pool.IsInUse(1), true);
    {
        const auto &cpool = pool; // force const container, prevent STL assertions
        ASSERT_EQ(cpool[0], 0);
        ASSERT_EQ(cpool[1], 1);
    }

    // Do some basic operations to make sure that it still works fine with zero as invalid index
    pool.Clear();
    ASSERT_EQ(pool.GetCount(), 0u);
    ASSERT_EQ(pool.size(), 1u); // index 0 is persistent (internal logic)

    const int index1 = pool.Add();
    const int index2 = pool.Add();
    const int index3 = pool.Add();
    ASSERT_EQ(pool.GetCount(), 3u); // counts only valid elements in use
    ASSERT_EQ(pool.size(), 4u); // index 0 is persistent (internal logic), and new index 1
    ASSERT_EQ(index1, 1);
    ASSERT_EQ(index2, 2);
    ASSERT_EQ(index3, 3);
    ASSERT_EQ(pool.IsFree(0), true);
    ASSERT_EQ(pool.IsFree(1), false);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(3), false);
    ASSERT_EQ(pool.IsInUse(0), false);
    ASSERT_EQ(pool.IsInUse(1), true);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(3), true);

    // Freeing forbidden index does nothing, it still cannot be used
    pool.Free(0);
    const int index4 = pool.Add();
    ASSERT_EQ(index4, 4);

    pool.Free(1);
    pool.Free(3);
    const int index5 = pool.Add();
    const int index6 = pool.Add();
    const int index7 = pool.Add();
    ASSERT_EQ(index5, 3);
    ASSERT_EQ(index6, 1);
    ASSERT_EQ(index7, 5);
}

TEST(IndexedObjectPool, InvalidIndexIsMax) {
    IndexedObjectPool<int, int, std::numeric_limits<int>::max()> pool;

    // Initial state
    ASSERT_EQ(pool.GetCount(), 0u);
    ASSERT_EQ(pool.size(), 0u);

    // Adding new elements
    const int index0 = pool.Add(10);
    const int index1 = pool.Add(20);
    const int index2 = pool.Add(30);
    ASSERT_EQ(pool.GetCount(), 3u); // only used elements
    ASSERT_EQ(pool.size(), 3u); // all elements
    ASSERT_EQ(index0, 0);
    ASSERT_EQ(index1, 1);
    ASSERT_EQ(index2, 2);
    ASSERT_EQ(pool.IsFree(0), false);
    ASSERT_EQ(pool.IsFree(1), false);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsInUse(0), true);
    ASSERT_EQ(pool.IsInUse(1), true);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool[0], 10);
    ASSERT_EQ(pool[1], 20);
    ASSERT_EQ(pool[2], 30);

    pool.Free(index1);
    ASSERT_EQ(pool.GetCount(), 2u);
    ASSERT_EQ(pool.IsFree(1), true);
    ASSERT_EQ(pool.IsInUse(1), false);


    const int index3 = pool.Add(40);
    const int index4 = pool.Add(50);
    ASSERT_EQ(pool.GetCount(), 4u);
    ASSERT_EQ(pool.IsFree(0), false);
    ASSERT_EQ(pool.IsFree(1), false);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(3), false);
    ASSERT_EQ(pool.IsInUse(0), true);
    ASSERT_EQ(pool.IsInUse(1), true);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(3), true);

    {
        const auto &cpool = pool; // force const container, prevent STL assertions
        ASSERT_EQ(cpool[1], 40);
        ASSERT_EQ(cpool[3], 50);
    }
    
    // Cleared state
    pool.Clear();
    ASSERT_EQ(pool.GetCount(), 0u);
    ASSERT_EQ(pool.size(), 0u);
}

TEST(IndexedObjectPool, FixedSlots) {
    IndexedObjectPool<int, int, -1> pool(5);

    // Initial state
    ASSERT_EQ(pool.GetCount(), 0u);
    ASSERT_EQ(pool.size(), 5u);
    ASSERT_EQ(pool.IsFree(0), true);
    ASSERT_EQ(pool.IsFree(1), true);
    ASSERT_EQ(pool.IsFree(2), true);
    ASSERT_EQ(pool.IsFree(3), true);
    ASSERT_EQ(pool.IsFree(4), true);
    ASSERT_EQ(pool.IsInUse(0), false);
    ASSERT_EQ(pool.IsInUse(1), false);
    ASSERT_EQ(pool.IsInUse(2), false);
    ASSERT_EQ(pool.IsInUse(3), false);
    ASSERT_EQ(pool.IsInUse(4), false);

    // New elements are added after fixed count
    const int index1 = pool.Add();
    const int index2 = pool.Add();
    const int index3 = pool.Add();
    ASSERT_EQ(pool.GetCount(), 3u);
    ASSERT_EQ(pool.size(), 8u);
    ASSERT_EQ(index1, 5);
    ASSERT_EQ(index2, 6);
    ASSERT_EQ(index3, 7);
    ASSERT_EQ(pool.IsFree(0), true);
    ASSERT_EQ(pool.IsFree(1), true);
    ASSERT_EQ(pool.IsFree(2), true);
    ASSERT_EQ(pool.IsFree(3), true);
    ASSERT_EQ(pool.IsFree(4), true);
    ASSERT_EQ(pool.IsFree(5), false);
    ASSERT_EQ(pool.IsFree(6), false);
    ASSERT_EQ(pool.IsFree(7), false);
    ASSERT_EQ(pool.IsInUse(0), false);
    ASSERT_EQ(pool.IsInUse(1), false);
    ASSERT_EQ(pool.IsInUse(2), false);
    ASSERT_EQ(pool.IsInUse(3), false);
    ASSERT_EQ(pool.IsInUse(4), false);
    ASSERT_EQ(pool.IsInUse(5), true);
    ASSERT_EQ(pool.IsInUse(6), true);
    ASSERT_EQ(pool.IsInUse(7), true);

    // Can set a fixed item directly, this marks them as used
    pool.Set(2, 2);
    pool.Set(4, 4);
    ASSERT_EQ(pool.GetCount(), 5u);
    ASSERT_EQ(pool.size(), 8u);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(4), false);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(4), true);
    ASSERT_EQ(pool[2], 2);
    ASSERT_EQ(pool[4], 4);

    // Freeing a fixed item still does not let add a new item there
    pool.Free(2);
    pool.Free(4);
    const int index4 = pool.Add();
    ASSERT_EQ(pool.GetCount(), 4u);
    ASSERT_EQ(pool.size(), 9u);
    ASSERT_EQ(index4, 8);

    // Clearing a pool still keeps fixed slots
    pool.Clear();
    ASSERT_EQ(pool.GetCount(), 0u);
    ASSERT_EQ(pool.size(), 5u);
}

TEST(IndexedObjectPool, SetFreeIndexes) {
    IndexedObjectPool<int, int, -1> pool;

    std::vector<int> elems = { 0,1,2,3,4,5,6,7 };
    std::vector<bool> is_used = { true, false, true, false, true, false, true, false };
    pool.Set(elems, is_used);

    ASSERT_EQ(pool.GetCount(), 4u);
    ASSERT_EQ(pool.size(), 8u);
    ASSERT_EQ(pool.IsFree(0), false);
    ASSERT_EQ(pool.IsFree(1), true);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(3), true);
    ASSERT_EQ(pool.IsFree(4), false);
    ASSERT_EQ(pool.IsFree(5), true);
    ASSERT_EQ(pool.IsFree(6), false);
    ASSERT_EQ(pool.IsFree(7), true);
    ASSERT_EQ(pool.IsInUse(0), true);
    ASSERT_EQ(pool.IsInUse(1), false);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(3), false);
    ASSERT_EQ(pool.IsInUse(4), true);
    ASSERT_EQ(pool.IsInUse(5), false);
    ASSERT_EQ(pool.IsInUse(6), true);
    ASSERT_EQ(pool.IsInUse(7), false);
    {
        const auto &cpool = pool; // force const container, prevent STL assertions
        ASSERT_EQ(cpool[0], 0);
        ASSERT_EQ(cpool[1], 1);
        ASSERT_EQ(cpool[2], 2);
        ASSERT_EQ(cpool[3], 3);
        ASSERT_EQ(cpool[4], 4);
        ASSERT_EQ(cpool[5], 5);
        ASSERT_EQ(cpool[6], 6);
        ASSERT_EQ(cpool[7], 7);
    }

    // The order of free indexes is reversed
    const int index1 = pool.Add();
    const int index2 = pool.Add();
    const int index3 = pool.Add();
    const int index4 = pool.Add();
    const int index5 = pool.Add();
    ASSERT_EQ(pool.GetCount(), 9u);
    ASSERT_EQ(pool.size(), 9u);
    ASSERT_EQ(index1, 7);
    ASSERT_EQ(index2, 5);
    ASSERT_EQ(index3, 3);
    ASSERT_EQ(index4, 1);
    ASSERT_EQ(index5, 8);
    ASSERT_EQ(pool.IsFree(0), false);
    ASSERT_EQ(pool.IsFree(1), false);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(3), false);
    ASSERT_EQ(pool.IsFree(4), false);
    ASSERT_EQ(pool.IsFree(5), false);
    ASSERT_EQ(pool.IsFree(6), false);
    ASSERT_EQ(pool.IsFree(7), false);
    ASSERT_EQ(pool.IsFree(8), false);
    ASSERT_EQ(pool.IsInUse(0), true);
    ASSERT_EQ(pool.IsInUse(1), true);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(3), true);
    ASSERT_EQ(pool.IsInUse(4), true);
    ASSERT_EQ(pool.IsInUse(5), true);
    ASSERT_EQ(pool.IsInUse(6), true);
    ASSERT_EQ(pool.IsInUse(7), true);
    ASSERT_EQ(pool.IsInUse(8), true);
}

TEST(IndexedObjectPool, SetFreeIndexesWithFixedSlots) {
    IndexedObjectPool<int, int, 0> pool(4);

    // Even if fixed slot is set to be free, it still cannot be re-used later,
    // (except when doing explicit Set()).
    std::vector<int> elems = { 0,1,2,3,4,5,6,7 };
    std::vector<bool> is_used = { true, false, true, false, true, false, true, false };
    pool.Set(elems, is_used);

    ASSERT_EQ(pool.GetCount(), 3u); // index 0 cannot be "used"
    ASSERT_EQ(pool.size(), 8u);
    ASSERT_EQ(pool.IsFree(0), true); // index 0 cannot be "used"
    ASSERT_EQ(pool.IsFree(1), true);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(3), true);
    ASSERT_EQ(pool.IsFree(4), false);
    ASSERT_EQ(pool.IsFree(5), true);
    ASSERT_EQ(pool.IsFree(6), false);
    ASSERT_EQ(pool.IsFree(7), true);
    ASSERT_EQ(pool.IsInUse(0), false); // index 0 cannot be "used"
    ASSERT_EQ(pool.IsInUse(1), false);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(3), false);
    ASSERT_EQ(pool.IsInUse(4), true);
    ASSERT_EQ(pool.IsInUse(5), false);
    ASSERT_EQ(pool.IsInUse(6), true);
    ASSERT_EQ(pool.IsInUse(7), false);
    {
        const auto &cpool = pool; // force const container, prevent STL assertions
        ASSERT_EQ(cpool[0], 0);
        ASSERT_EQ(cpool[1], 1);
        ASSERT_EQ(cpool[2], 2);
        ASSERT_EQ(cpool[3], 3);
        ASSERT_EQ(cpool[4], 4);
        ASSERT_EQ(cpool[5], 5);
        ASSERT_EQ(cpool[6], 6);
        ASSERT_EQ(cpool[7], 7);
    }

    // The order of free indexes is reversed;
    // fixed slots cannot be acquired this way
    const int index1 = pool.Add();
    const int index2 = pool.Add();
    const int index3 = pool.Add();
    const int index4 = pool.Add();
    const int index5 = pool.Add();
    ASSERT_EQ(pool.GetCount(), 8u);
    ASSERT_EQ(pool.size(), 11u);
    ASSERT_EQ(index1, 7);
    ASSERT_EQ(index2, 5);
    ASSERT_EQ(index3, 8);
    ASSERT_EQ(index4, 9);
    ASSERT_EQ(index5, 10);
    ASSERT_EQ(pool.IsFree(0), true); // index 0 cannot be "used"
    ASSERT_EQ(pool.IsFree(1), true);
    ASSERT_EQ(pool.IsFree(2), false);
    ASSERT_EQ(pool.IsFree(3), true);
    ASSERT_EQ(pool.IsFree(4), false);
    ASSERT_EQ(pool.IsFree(5), false);
    ASSERT_EQ(pool.IsFree(6), false);
    ASSERT_EQ(pool.IsFree(7), false);
    ASSERT_EQ(pool.IsFree(8), false);
    ASSERT_EQ(pool.IsFree(9), false);
    ASSERT_EQ(pool.IsFree(10), false);
    ASSERT_EQ(pool.IsInUse(0), false); // index 0 cannot be "used"
    ASSERT_EQ(pool.IsInUse(1), false);
    ASSERT_EQ(pool.IsInUse(2), true);
    ASSERT_EQ(pool.IsInUse(3), false);
    ASSERT_EQ(pool.IsInUse(4), true);
    ASSERT_EQ(pool.IsInUse(5), true);
    ASSERT_EQ(pool.IsInUse(6), true);
    ASSERT_EQ(pool.IsInUse(7), true);
    ASSERT_EQ(pool.IsInUse(8), true);
    ASSERT_EQ(pool.IsInUse(9), true);
    ASSERT_EQ(pool.IsInUse(10), true);
}