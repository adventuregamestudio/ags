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
// A plugin's dynamic manager wrapper.
// The purpose is to hide the actual plugin's manager behind a proxy,
// as plugin's interface may not fully comply to our internal one.
// This prevents errors if one of the extended methods is called by the engine.
// The base ICCDynamicObject interface currently consists of only few
// methods, which are used only once in the object's lifetime,
// and therefore this wrapper should have a small overhead.
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__PLUGINOBJECT_H
#define __AGS_EE_DYNOBJ__PLUGINOBJECT_H

#include "ac/dynobj/cc_agsdynamicobject.h"


struct CCPluginObject final : CCBasicObject
{
private:
    virtual ~CCPluginObject() = default;

    IScriptObject *_pluginMgr = nullptr;

public:
    CCPluginObject(IScriptObject *plugin_mgr)
        : _pluginMgr(plugin_mgr) {}

    // Dispose the object
    int Dispose(void *address, bool force) override
    {
        // At the moment this wrapper's lifetime is tied to the plugin object
        if (_pluginMgr->Dispose(address, force) != 0)
        {
            delete this;
            return 1;
        }
        return 0;
    }
    // Return the type name of the object
    const char *GetType() override
    {
        return _pluginMgr->GetType();
    }
    // Serialize the object into BUFFER (which is BUFSIZE bytes)
    // return number of bytes used
    int Serialize(void *address, uint8_t *buffer, int bufsize) override
    {
        return _pluginMgr->Serialize(address, buffer, bufsize);
    }
};

#endif // __AGS_EE_DYNOBJ__PLUGINOBJECT_H