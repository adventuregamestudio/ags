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
#include <allegro.h>

class SpriteImportEnv : public ::testing::Environment
{
public:
    void SetUp() override
    {
        // Init Allegro RGB shifts; necessary for doing color conversions
        set_rgb_shifts(10, 5, 0, 11, 5, 0, 16, 8, 0, 16, 8, 0, 24);
        // Force internal "bestfit" table to initialize, as our tests may be
        // run in parallel, having it initialized during the test may cause
        // race condition.
        PALETTE dummy;
        bestfit_color(dummy, 0, 0, 0);
    }

    void TearDown() override
    {
    }
};
