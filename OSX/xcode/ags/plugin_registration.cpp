//
//  plugin_registration.cpp
//  Shivah
//
//  Created by Nick Sonneveld on 26/06/2016.
//
//

#include "plugin_registration.hpp"

#include "plugin/agsplugin.h"
#include "plugin/plugin_builtin.h"
//#include "example.h"

typedef void (*t_engine_pre_init_callback)(void);
extern void engine_set_pre_init_callback(t_engine_pre_init_callback callback);

static void pl_register_builtin_plugin() {

//  pl_register_builtin_plugin((InbuiltPluginDetails){
//    .filename = "example",
//    .engineStartup = example::AGS_EngineStartup,
//    .engineShutdown = example::AGS_EngineShutdown,
//    .onEvent = example::AGS_EngineOnEvent,
//    .initGfxHook = example::AGS_EngineInitGfx,
//    .debugHook = example::AGS_EngineDebugHook,
//  });

}


__attribute__((constructor))
static void oninit_register_builtin_plugins() {
  engine_set_pre_init_callback(&pl_register_builtin_plugin);
}
