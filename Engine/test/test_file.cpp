
#ifdef _DEBUG

#include <stdio.h>
#include <string.h>
#include "util/alignedstream.h"
#include "util/filestream.h"
#include "debug/assert.h"

using AGS::Common::CString;
using AGS::Common::CDataStream;
using AGS::Common::CAlignedStream;
namespace File = AGS::Common::File;

struct TTrickyAlignedData
{
    char    a;
    int     b;
    int     c;
    short   d[3];
    int     e;
    char    f[17];
    int     g[4];
    short   h[13];
};

void Test_File()
{
    //-----------------------------------------------------
    // Operations
    CDataStream *out = File::OpenFile("test.tmp", AGS::Common::kFile_CreateAlways, AGS::Common::kFile_Write);

    out->WriteInt16(10);
    out->WriteInt64(-20202);
    out->WriteString("test.tmp");

    TTrickyAlignedData tricky_data_out;
    {
        tricky_data_out.a = 11;
        tricky_data_out.b = 12;
        tricky_data_out.c = 13;
        tricky_data_out.d[0] = 14;
        tricky_data_out.d[1] = 15;
        tricky_data_out.d[2] = 16;
        tricky_data_out.e = 17;
        memset(tricky_data_out.f, 0, 17);
        tricky_data_out.g[0] = 18;
        tricky_data_out.g[1] = 19;
        tricky_data_out.g[2] = 20;
        tricky_data_out.g[3] = 21;
        memset(tricky_data_out.h, 0, 13 * sizeof(short));
        out->Write(&tricky_data_out, sizeof(TTrickyAlignedData));
    }

    out->WriteInt32(20);

    intptr_var_t ptr32_array_out[4];
    ptr32_array_out[0] = 0xABCDABCD;
    ptr32_array_out[1] = 0xFEDCFEDC;
    ptr32_array_out[2] = 0xFEEDBEEF;
    ptr32_array_out[3] = 0xBEEFFEED;
    out->WriteArrayOfIntPtr32(ptr32_array_out, 4);

    delete out;

    //-------------------------------------------------------------------------

    CDataStream *in = File::OpenFile("test.tmp", AGS::Common::kFile_Open, AGS::Common::kFile_Read);

    int16_t int16val    = in->ReadInt16();
    int64_t int64val    = in->ReadInt64();
    CString str         = in->ReadString();

    TTrickyAlignedData tricky_data_in;
    {
        CAlignedStream as(in, AGS::Common::kAligned_Read);
        tricky_data_in.a = as.ReadInt8();
        tricky_data_in.b = as.ReadInt32();
        tricky_data_in.c = as.ReadInt32();
        as.ReadArrayOfInt16(tricky_data_in.d, 3);
        tricky_data_in.e = as.ReadInt32();
        as.Read(tricky_data_in.f, 17);
        as.ReadArrayOfInt32(tricky_data_in.g, 4);
        as.ReadArrayOfInt16(tricky_data_in.h, 13);
        as.ReleaseStream(); // releasing filestream so that it won't be deleted
    }

    int32_t int32val    = in->ReadInt32();

    intptr_var_t ptr32_array_in[4];
    in->ReadArrayOfIntPtr32(ptr32_array_in, 4);

    delete in;

    //-----------------------------------------------------
    // Assertions
    assert(int16val == 10);
    assert(int64val == -20202);
    assert(strcmp(str.GetCStr(), "test.tmp") == 0);
    assert(memcmp(&tricky_data_in, &tricky_data_out, sizeof(TTrickyAlignedData)) == 0);
    assert(int32val == 20);

    assert(ptr32_array_in[0] == 0xABCDABCD);
    assert(ptr32_array_in[1] == 0xFEDCFEDC);
    assert(ptr32_array_in[2] == 0xFEEDBEEF);
    assert(ptr32_array_in[3] == 0xBEEFFEED);
}

#endif // _DEBUG
