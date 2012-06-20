#ifndef __AC_SCRIPTVIEWFRAME_H
#define __AC_SCRIPTVIEWFRAME_H

#include "ac/dynobj/cc_agsdynamicobject.h"

struct ScriptViewFrame : AGSCCDynamicObject {
    int view, loop, frame;

    virtual int Dispose(const char *address, bool force);
    virtual const char *GetType();
    virtual int Serialize(const char *address, char *buffer, int bufsize);
    virtual void Unserialize(int index, const char *serializedData, int dataSize);

    ScriptViewFrame(int p_view, int p_loop, int p_frame);
    ScriptViewFrame();
};

#endif // __AC_SCRIPTVIEWFRAME_H