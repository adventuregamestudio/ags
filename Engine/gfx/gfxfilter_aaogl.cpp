//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================

#include "core/platform.h"

#if AGS_HAS_OPENGL

#include "gfx/gfxfilter_aaogl.h"
#include "ogl_headers.h"

namespace AGS
{
namespace Engine
{
namespace OGL
{

const GfxFilterInfo AAOGLGfxFilter::FilterInfo = GfxFilterInfo("Linear", "Linear interpolation");

bool AAOGLGfxFilter::UseLinearFiltering() const
{
    return true;
}

void AAOGLGfxFilter::SetFilteringForStandardSprite()
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

const GfxFilterInfo &AAOGLGfxFilter::GetInfo() const
{
    return FilterInfo;
}

} // namespace OGL
} // namespace Engine
} // namespace AGS

#endif // AGS_HAS_OPENGL
