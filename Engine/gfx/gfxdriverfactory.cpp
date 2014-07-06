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

#include "gfx/gfxdriverfactory.h"
#ifdef WINDOWS_VERSION
#include "platform/windows/gfx/ali3dd3d.h"
#endif
#include "gfx/ali3dsw.h"
#include "gfx/ali3dogl.h"

extern int psp_gfx_renderer;

namespace AGS
{
namespace Engine
{

IGfxDriverFactory *GetGfxDriverFactory(const String id)
{
#ifdef WINDOWS_VERSION
    if (id.CompareNoCase("D3D9") == 0)
        return D3D::D3DGraphicsFactory::GetFactory();
#endif
#if defined (ANDROID_VERSION) || defined (IOS_VERSION) || defined (WINDOWS_VERSION)
    if ((id.CompareNoCase("OGL") != 0 || id.CompareNoCase("D3D9") == 0) && psp_gfx_renderer > 0)
        return OGL::OGLGraphicsFactory::GetFactory();
#endif
    if (id.CompareNoCase("DX5") == 0)
        return ALSW::ALSWGraphicsFactory::GetFactory();
    return NULL;
}

} // namespace Engine
} // namespace AGS
