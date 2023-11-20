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
// Plugin system functions.
//
//=============================================================================
#ifndef __AGS_EE_PLUGIN__PLUGINENGINE_H
#define __AGS_EE_PLUGIN__PLUGINENGINE_H

#include <vector>
#include "ac/dynobj/cc_scriptobject.h"
#include "game/game_init.h"
#include "game/plugininfo.h"
#include "util/string.h"
namespace AGS { namespace Common { class Stream; }}
using namespace AGS; // FIXME later


//
// PluginObjectReader is a managed object unserializer registered by plugin.
//
struct PluginObjectReader
{
    const Common::String Type;
    ICCObjectReader *const Reader = nullptr;

    PluginObjectReader(const Common::String &type, ICCObjectReader *reader)
        : Type(type), Reader(reader) {}
};


void pl_stop_plugins();
void pl_startup_plugins();
int  pl_run_plugin_hooks (int event, int data);
void pl_run_plugin_init_gfx_hooks(const char *driverName, void *data);
int  pl_run_plugin_debug_hooks (const char *scriptfile, int linenum);
// Tries to register plugins, either by loading dynamic libraries, or getting any kind of replacement
Engine::GameInitError pl_register_plugins(const std::vector<Common::PluginInfo> &infos);
bool pl_is_plugin_loaded(const char *pl_name);

//returns whether _any_ plugins want a particular event
bool pl_any_want_hook(int event);

bool RegisterPluginStubs(const char* name);
#endif // __AGS_EE_PLUGIN__PLUGINENGINE_H
