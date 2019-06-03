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

#ifndef __AGS_EE_GAME__SAVEGAMECOMPONENTS_H
#define __AGS_EE_GAME__SAVEGAMECOMPONENTS_H

#include <memory>
#include "game/savegame.h"
#include "game/savegame_internal.h"
#include "gfx/bitmap.h"
#include "util/stream.h"
#include "util/string.h"

namespace AGS
{
namespace Engine
{

using Common::Bitmap;
using Common::Stream;
using Common::String;

typedef std::shared_ptr<Stream> PStream;

namespace SavegameComponents
{
    // Reads all available components from the stream
    HSaveError    ReadAll(PStream in, SavegameVersion svg_version, const PreservedParams &pp, RestoredData &r_data);
    // Writes a full list of common components to the stream
    HSaveError    WriteAllCommon(PStream out);
}

} // namespace Engine
} // namespace AGS

#endif // __AGS_EE_GAME__SAVEGAMECOMPONENTS_H
