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
#include "game/interactions.h"

namespace AGS
{
namespace Common
{

InteractionScripts *InteractionScripts::CreateFromStream(Stream *in)
{
    const size_t evt_count = in->ReadInt32();
    InteractionScripts *scripts = new InteractionScripts();
    for (size_t i = 0; i < evt_count; ++i)
    {
        String name = String::FromStream(in);
        scripts->ScriptFuncNames.push_back(name);
    }
    return scripts;
}

} // namespace Common
} // namespace AGS
