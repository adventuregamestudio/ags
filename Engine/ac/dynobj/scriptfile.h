
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTFILE_H
#define __AGS_EE_DYNOBJ__SCRIPTFILE_H

#include "ac/dynobj/cc_dynamicobject.h"

namespace AGS { namespace Common { class CDataStream; } }
using namespace AGS; // FIXME later

#define scFileRead   1
#define scFileWrite  2
#define scFileAppend 3

struct sc_File : ICCDynamicObject {
    Common::CDataStream *handle;

    static const Common::FileOpenMode fopenModes[];
    static const Common::FileWorkMode fworkModes[];

    virtual int Dispose(const char *address, bool force);

    virtual const char *GetType();

    virtual int Serialize(const char *address, char *buffer, int bufsize);

    int OpenFile(const char *filename, int mode);
    void Close();

    sc_File();
};

#endif // __AGS_EE_DYNOBJ__SCRIPTFILE_H
