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
// API for apps to register their own inbuilt plugins.
//
//=============================================================================
#ifndef __AGS_EE_PLUGIN__PLUGINBUILTIN_H
#define __AGS_EE_PLUGIN__PLUGINBUILTIN_H

class IAGSEngine;

namespace AGS { namespace Common { class String; }}
struct InbuiltPluginDetails {
    AGS::Common::String    filename;
    void      (*engineStartup) (IAGSEngine *);
    void      (*engineShutdown) ();
    int       (*onEvent) (int, int);
    void      (*initGfxHook) (const char *driverName, void *data);
    int       (*debugHook) (const char * whichscript, int lineNumber, int reserved);
};

// Register a builtin plugin.
int pl_register_builtin_plugin(InbuiltPluginDetails const &details);

#endif // __AGS_EE_PLUGIN__PLUGINBUILTIN_H
