#ifndef AGS_SNOWRAIN_H
#define AGS_SNOWRAIN_H

#include "plugin/agsplugin.h"

namespace ags_snowrain
{
  void AGS_EngineStartup(IAGSEngine *lpEngine);
  void AGS_EngineShutdown();
  intptr_t AGS_EngineOnEvent(int event, intptr_t data);
  int AGS_EngineDebugHook(const char *scriptName, int lineNum, int reserved);
  void AGS_EngineInitGfx(const char *driverID, void *data);
}

#endif