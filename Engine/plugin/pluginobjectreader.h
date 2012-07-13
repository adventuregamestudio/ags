
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_PLUGIN__PLUGINOBJECTREADER_H
#define __AGS_EE_PLUGIN__PLUGINOBJECTREADER_H

#ifndef _AGS_PLUGIN_H
#define IAGSManagedObjectReader void
#endif

struct PluginObjectReader {
    IAGSManagedObjectReader *reader;
    const char *type;
};

#ifndef _AGS_PLUGIN_H
#undef IAGSManagedObjectReader
#endif

#endif // __AGS_EE_PLUGIN__PLUGINOBJECTREADER_H
