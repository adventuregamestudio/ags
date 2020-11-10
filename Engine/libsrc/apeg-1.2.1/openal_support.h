#ifndef APEG_OPENAL_SUPPORT_H__
#define APEG_OPENAL_SUPPORT_H__

#ifdef __APPLE__
    #include <OpenAL/OpenAL.h>
#else
    #include "AL/al.h"
    #include "AL/alc.h"
    // [sonneveld] disabled until I add extension detection
    // #include "AL/alext.h"
#endif

#endif
