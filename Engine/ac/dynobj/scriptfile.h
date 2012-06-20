
//=============================================================================
//
//
//
//=============================================================================
#ifndef __AGS_EE_DYNOBJ__SCRIPTFILE_H
#define __AGS_EE_DYNOBJ__SCRIPTFILE_H

#include "ac/dynobj/cc_dynamicobject.h"

#define scFileRead   1
#define scFileWrite  2
#define scFileAppend 3

struct sc_File : ICCDynamicObject {
    FILE *handle;

	static const char *fopenModes[];

    virtual int Dispose(const char *address, bool force);

    virtual const char *GetType();

    virtual int Serialize(const char *address, char *buffer, int bufsize);

    int OpenFile(const char *filename, int mode);
    void Close();

    sc_File();
};

#endif // __AGS_EE_DYNOBJ__SCRIPTFILE_H
