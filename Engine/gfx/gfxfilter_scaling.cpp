//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-20xx others
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// http://www.opensource.org/licenses/artistic-license-2.0.php
//
//=============================================================================

#include "gfx/gfxfilter_scaling.h"
#include "util/wgt2allg.h"
#include "device/mousew32.h"

namespace AGS
{
namespace Engine
{

ScalingGfxFilter::ScalingGfxFilter(int multiplier)
{
    MULTIPLIER = multiplier;
}

} // namespace Engine
} // namespace AGS
