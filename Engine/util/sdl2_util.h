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
//
// Custom SDL2 utilities, wrappers, reimplementations etc.
//
//=============================================================================
#ifndef __AGS_EE_UTIL__SDL2UTIL_H
#define __AGS_EE_UTIL__SDL2UTIL_H

#include <memory>
#include <SDL.h>
#include "util/stream.h"

namespace AGS
{
namespace Engine
{
namespace SDL2Util
{
    // Opens a custom SDL2 RWOps implementation that embeds AGS stream
    SDL_RWops *OpenRWops(std::unique_ptr<Common::Stream> ags_stream);
} // namespace SDL2Util
} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_UTIL__RWOPSUTIL_H
