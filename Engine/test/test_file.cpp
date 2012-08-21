
#ifdef _DEBUG

#include <stdio.h>
#include <string.h>
#include "util/filestream.h"
#include "test/test_internal.h"

using AGS::Common::CString;
using AGS::Common::CStream;
namespace File = AGS::Common::File;

void Test_File()
{
    return;
    //-----------------------------------------------------
    // Operations
    CStream *out = File::OpenFile("test.tmp", AGS::Common::kFile_CreateAlways, AGS::Common::kFile_Write);

    out->WriteInt16(10);
    out->WriteInt64(-20202);
    out->WriteString("test.tmp");

    delete out;

    CStream *in = File::OpenFile("test.tmp", AGS::Common::kFile_Open, AGS::Common::kFile_Read);

    int16_t int16val    = in->ReadInt16();
    int64_t int64val    = in->ReadInt64();
    CString str         = in->ReadString();

    delete in;

    //-----------------------------------------------------
    // Assertions
    assert(int16val == 10);
    assert(int64val == -20202);
    assert(strcmp(str.GetCStr(), "test.tmp") == 0);        
}

#endif // _DEBUG
