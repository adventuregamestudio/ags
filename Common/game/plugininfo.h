//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2025 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
//
// PluginInfo - a struct defining general information on game plugin.
//
//=============================================================================

#ifndef __AGS_CN_GAME__PLUGININFO_H
#define __AGS_CN_GAME__PLUGININFO_H

#include <memory>
#include "util/string.h"

// TODO: why 10 MB limit?
#define PLUGIN_SAVEBUFFERSIZE 10247680

namespace AGS
{
namespace Common
{

struct PluginInfo
{
    // (File)name of plugin
    String      Name;
    // Custom data for plugin
    std::vector<uint8_t> Data;

    PluginInfo() = default;
};

} // namespace Common
} // namespace AGS

#endif // __AGS_CN_GAME__PLUGININFO_H
