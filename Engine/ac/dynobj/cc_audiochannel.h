
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__CCAUDIOCHANNEL_H
#define __AGS_EE_DYNOBJ__CCAUDIOCHANNEL_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct CCAudioChannel : AGSCCDynamicObject {
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);
};

#endif // __AGS_EE_DYNOBJ__CCAUDIOCHANNEL_H
