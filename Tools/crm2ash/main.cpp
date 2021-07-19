#include <stdio.h>
#include "data/room_utils.h"
#include "data/scriptgen.h"
#include "util/data_ext.h"
#include "util/file.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;


class RoomScNamesReader : public DataExtReader
{
public:
    RoomScNamesReader(RoomScNames &data, RoomFileVersion data_ver, Stream *in)
        : DataExtReader(in,
            kDataExt_NumID8 | ((data_ver < kRoomVersion_350) ? kDataExt_File32 : kDataExt_File64))
        , _data(data)
        , _dataVer(data_ver)
    {}

private:
    HError ReadBlock(int block_id, const String &ext_id,
        soff_t block_len, bool &read_next) override
    {
        return ReadRoomScNames(_data, in, (RoomFileBlock)block_id, ext_id, block_len, _dataVer);
    }

    RoomScNames &_data;
    RoomFileVersion _dataVer;
};


const char *HELP_STRING = "Usage: crm2ash <input-room.crm> <output-room.ash>\n";

int main(int argc, char *argv[])
{
    printf("crm2ash v0.1.0 - AGS compiled room's script header generator\n"\
        "Copyright (c) 2021 AGS Team and contributors\n");
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (ags_stricmp(arg, "--help") == 0 || ags_stricmp(arg, "/?") == 0 || ags_stricmp(arg, "-?") == 0)
        {
            printf("%s\n", HELP_STRING);
            return 0; // display help and bail out
        }
    }
    if (argc < 3)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    const char *src = argv[1];
    const char *dst = argv[2];
    printf("Input room file: %s\n", src);
    printf("Output script header: %s\n", dst);

    //-----------------------------------------------------------------------//
    // Read room struct
    //-----------------------------------------------------------------------//
    RoomDataSource datasrc;
    HError err = OpenRoomFile(src, datasrc);
    if (!err)
    {
        printf("Error: failed to open room file for reading:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    
    RoomScNames data;
    RoomScNamesReader reader(data, datasrc.DataVersion, datasrc.InputStream.get());
    err = reader.Read();
    if (!err)
    {
        printf("Error: failed to read room file:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    datasrc.InputStream.reset();

    //-----------------------------------------------------------------------//
    // Create script header
    //-----------------------------------------------------------------------//
    String header = MakeRoomScriptHeader(data);

    //-----------------------------------------------------------------------//
    // Write script header
    //-----------------------------------------------------------------------//
    Stream *out = File::CreateFile(dst);
    if (!out)
    {
        printf("Error: failed to open script header for writing.\n");
        return -1;
    }
    out->Write(header.GetCStr(), header.GetLength());
    delete out;
    printf("Script header written successfully.\nDone.\n");
    return 0;
}
