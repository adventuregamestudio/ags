//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_PLATFORM__AGS32BITOSDRIVER_H
#define __AGS_EE_PLATFORM__AGS32BITOSDRIVER_H

#include "platform/agsplatformdriver.h"

struct AGS32BitOSDriver : AGSPlatformDriver {
    virtual void GetSystemTime(ScriptDateTime*) ;
    virtual void YieldCPU();
};

int cd_player_init();
int cd_player_control(int cmdd, int datt);

#endif // __AGS_EE_PLATFORM__AGS32BITOSDRIVER_H
