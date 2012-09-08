
#define THIS_IS_THE_PLUGIN
#include "plugin/agsplugin.h"

void AGS_EngineStartup(IAGSEngine *lpEngine)
{
}

void AGS_EngineShutdown()
{
}

int AGS_EngineOnEvent(int event, int data)
{
  return 0;
}

int AGS_EngineDebugHook(const char *scriptName, int lineNum, int reserved)
{
  return 0;
}

void AGS_EngineInitGfx(const char *driverID, void *data)
{
}
