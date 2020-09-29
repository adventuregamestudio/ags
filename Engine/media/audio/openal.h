#ifndef __AGS_EE_MEDIA_OPENAL_H__
#define __AGS_EE_MEDIA_OPENAL_H__
#include "core/platform.h"

#if AGS_PLATFORM_OS_MACOS
    #include <OpenAL/OpenAL.h>
#else
    #include <AL/al.h>
    #include <AL/alc.h>
    // [sonneveld] disabled until I add extension detection
    // #include "alext.h"
#endif

#endif // __AGS_EE_MEDIA_OPENAL_H__
