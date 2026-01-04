//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2026 various contributors
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
intptr_t pl_run_plugin_hooks(int event, intptr_t data);
void pl_run_plugin_init_gfx_hooks(const char *driverName, void *data);
int  pl_run_plugin_debug_hooks(const char *scriptfile, int linenum);
// Finds a plugin that wants this event, starting with pl_index;
// returns TRUE on success and fills its index and name;
// returns FALSE if no more suitable plugins found.
bool pl_query_next_plugin_for_event(int event, uint32_t &pl_index, AGS::Common::String &pl_name);
// Runs event for a plugin identified by an index it was registered under.
intptr_t pl_run_plugin_hook_by_index(uint32_t pl_index, int event, intptr_t data);
// Runs event for a plugin identified by its name.
intptr_t pl_run_plugin_hook_by_name(AGS::Common::String &pl_name, int event, intptr_t data);

// Tries to register plugins, either by loading dynamic libraries, or getting any kind of replacement
AGS::Engine::GameInitError pl_register_plugins(const std::vector<AGS::Common::PluginInfo> &infos, bool enable_load);
bool pl_is_plugin_loaded(const char *pl_name);

//returns whether _any_ plugins want a particular event
bool pl_any_want_hook(int event);

bool RegisterPluginStubs(const char* name);

#endif // __AGS_EE_PLUGIN__PLUGINENGINE_H
