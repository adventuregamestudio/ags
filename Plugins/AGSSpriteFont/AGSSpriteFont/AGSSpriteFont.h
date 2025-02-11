#ifndef AGS_SPRITE_FONT_H
#define AGS_SPRITE_FONT_H

#include "plugin/agsplugin.h"

namespace agsspritefont
{
  void AGS_EngineStartup(IAGSEngine *lpEngine);
  void AGS_EngineShutdown();
  intptr_t AGS_EngineOnEvent(int event, intptr_t data);
}

#endif
