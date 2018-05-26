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
#include "gfx/ali3dsw.h"
#include "gfx/gfxfilter_allegro.h"

#if defined(WINDOWS_VERSION) || defined(ANDROID_VERSION) || defined(IOS_VERSION)
#include "gfx/ali3dogl.h" // TODO: support on Linux too
#include "gfx/gfxfilter_ogl.h"
#endif

#if defined(WINDOWS_VERSION)
#include "platform/windows/gfx/ali3dd3d.h"
#include "gfx/gfxfilter_d3d.h"
#endif

#include "main/main_allegro.h"

namespace AGS
{
namespace Engine
{

void GetGfxDriverFactoryNames(StringV &ids)
{
#ifdef WINDOWS_VERSION
    ids.push_back("D3D9");
#endif
#if defined (ANDROID_VERSION) || defined (IOS_VERSION) || defined (WINDOWS_VERSION)
    ids.push_back("OGL");
#endif
    ids.push_back("Software");
}

IGfxDriverFactory *GetGfxDriverFactory(const String id)
{
#ifdef WINDOWS_VERSION
    if (id.CompareNoCase("D3D9") == 0)
        return D3D::D3DGraphicsFactory::GetFactory();
#endif
#if defined (ANDROID_VERSION) || defined (IOS_VERSION)|| defined (WINDOWS_VERSION)
    if (id.CompareNoCase("OGL") == 0)
        return OGL::OGLGraphicsFactory::GetFactory();
#endif
    if (id.CompareNoCase("Software") == 0)
        return ALSW::ALSWGraphicsFactory::GetFactory();
    set_allegro_error("No graphics factory with such id: %s", id.GetCStr());
    return NULL;
}

} // namespace Engine
} // namespace AGS
