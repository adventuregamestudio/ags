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
#define NOMINMAX
#include "gfx/gfxdriverfactory.h"

#include "core/platform.h"

#include "gfx/ali3dsw.h"
#include "gfx/gfxfilter_sdl_renderer.h"

#if AGS_HAS_OPENGL
#include "gfx/ali3dogl.h"
#include "gfx/gfxfilter_ogl.h"
#endif

#if AGS_HAS_DIRECT3D
#include "platform/windows/gfx/ali3dd3d.h"
#include "gfx/gfxfilter_d3d.h"
#endif

namespace AGS
{
namespace Engine
{

void GetGfxDriverFactoryNames(StringV &ids)
{
#if AGS_HAS_DIRECT3D
    ids.push_back("D3D9");
#endif
#if AGS_HAS_OPENGL
    ids.push_back("OGL");
#endif
    ids.push_back("Software");
}

IGfxDriverFactory *GetGfxDriverFactory(const String id)
{
#if AGS_HAS_DIRECT3D
    if (id.CompareNoCase("D3D9") == 0)
        return D3D::D3DGraphicsFactory::GetFactory();
#endif
#if AGS_HAS_OPENGL
    if (id.CompareNoCase("OGL") == 0)
        return OGL::OGLGraphicsFactory::GetFactory();
#endif
    if (id.CompareNoCase("Software") == 0)
        return ALSW::SDLRendererGraphicsFactory::GetFactory();
    SDL_SetError("No graphics factory with such id: %s", id.GetCStr());
    return nullptr;
}

} // namespace Engine
} // namespace AGS
