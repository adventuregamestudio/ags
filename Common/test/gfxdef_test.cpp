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
