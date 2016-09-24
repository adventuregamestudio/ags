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
//
// PluginInfo - a struct defining general information on game plugin.
//
//=============================================================================

#ifndef __AGS_CN_GAME__PLUGININFO_H
#define __AGS_CN_GAME__PLUGININFO_H

#include "util/stdtr1compat.h"
#include TR1INCLUDE(memory)
#include "util/string.h"

namespace AGS
{
namespace Common
{

struct PluginInfo
{
    // (File)name of plugin
    String      Name;
    // Custom data for plugin
    stdtr1compat::shared_ptr<char> Data;
    size_t      DataLen;

    PluginInfo() : DataLen(0) {}
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__PLUGININFO_H
