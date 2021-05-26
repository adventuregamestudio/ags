#include <stdio.h>
#include "data/room_utils.h"
#include "data/scriptgen.h"
#include "util/file.h"
#include "util/string_compat.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;


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
    auto err = OpenRoomFile(src, datasrc);
    if (!err)
    {
        printf("Error: failed to open room file for reading:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    
    RoomScNames data;
    auto read_cb = [&data](Stream *in, RoomFileBlock block, const String &ext_id,
        soff_t block_len, RoomFileVersion data_ver, bool &read_next)
        { return ReadRoomScNames(data, in, block, ext_id, block_len, data_ver); };
    err = ReadRoomData(read_cb, datasrc.InputStream.get(), datasrc.DataVersion);
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
