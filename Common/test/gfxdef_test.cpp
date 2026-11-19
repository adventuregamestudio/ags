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
#include "gfx/gfx_def.h"

using namespace AGS::Common;

TEST(GfxDef, LegacyTransparency) {
    // Test that every transparency which is a multiple of 10 is converted
    // forth and back without loosing precision
    const size_t arr_sz = 11;
    const int trans100[arr_sz] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100 };
    int trans255[arr_sz] = { 0 };
    int trans100_back[arr_sz] = { 0 };

    for (int i = 0; i < arr_sz; ++i)
    {
        trans255[i] = GfxDef::Trans100ToLegacyTrans255(trans100[i]);
        trans100_back[i] = GfxDef::LegacyTrans255ToTrans100(trans255[i]);
        ASSERT_TRUE(trans100[i] == trans100_back[i]);
    }
}

// The 4.0 Transparency script property over opacity reads back what AGS 4.0 read back, which kept
// the legacy 255-ranged transparency
TEST(GfxDef, Trans100OverOpacity) {
    ASSERT_EQ(GfxDef::Trans100ToOpacity(0), 255);
    ASSERT_EQ(GfxDef::Trans100ToOpacity(100), 0);
    ASSERT_EQ(GfxDef::OpacityToTrans100(255), 0);
    ASSERT_EQ(GfxDef::OpacityToTrans100(0), 100);
    for (int t = 0; t <= 100; ++t)
    {
        const int legacy = GfxDef::Trans100ToLegacyTrans255(t);
        ASSERT_EQ(GfxDef::Trans100ToOpacity(t), GfxDef::LegacyTrans255ToAlpha255(legacy));
        ASSERT_EQ(GfxDef::OpacityToTrans100(GfxDef::Trans100ToOpacity(t)), GfxDef::LegacyTrans255ToTrans100(legacy));
    }
    ASSERT_EQ(GfxDef::OpacityToTrans100(GfxDef::Trans100ToOpacity(37)), 38); // as in 4.0
    for (int a = 0; a <= 255; ++a)
    {
        const int t = GfxDef::OpacityToTrans100(a);
        ASSERT_TRUE(t >= 0 && t <= 100);
    }
}
