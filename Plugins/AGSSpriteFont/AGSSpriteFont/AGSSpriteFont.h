#ifndef AGS_SPRITEFONT_H
#define AGS_SPRITEFONT_H

#include "plugin/agsplugin.h"

namespace ags_spritefont
{
  void AGS_EngineStartup(IAGSEngine *lpEngine);
  void AGS_EngineShutdown();
  int AGS_EngineOnEvent(int event, int data);
  int AGS_EngineDebugHook(const char *scriptName, int lineNum, int reserved);
  void AGS_EngineInitGfx(const char *driverID, void *data);
}

#endif