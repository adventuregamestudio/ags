#include <stdio.h>
#include "data/tra_utils.h"
#include "util/file.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;


const char *HELP_STRING = "Usage: trac <input.trs> <output.tra>\n\t[--gamename <name>][--uniqueid <idnum>]";

int main(int argc, char *argv[])
{
    printf("trac v0.1.0 - AGS translation compiler (TRS -> TRA)\n"\
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

    String game_name;
    int game_uid;
    for (int i = 3; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (ags_stricmp(arg, "--gamename") == 0 && (i < argc - 1))
            game_name = argv[++i];
        else if (ags_stricmp(arg, "--uniqueid") == 0 && (i < argc - 1))
            game_uid = StrUtil::StringToInt(argv[++i]);
    }

    const char *src = argv[1];
    const char *dst = argv[2];
    printf("Input translation source: %s\n", src);
    printf("Output compiled translation: %s\n", dst);
    printf("Game name: %s\n", game_name.GetCStr());
    printf("Game uniqueid: %d\n", game_uid);

    //-----------------------------------------------------------------------//
    // Read TRS
    //-----------------------------------------------------------------------//
    Stream *in = File::OpenFileRead(src);
    if (!in)
    {
        printf("Error: failed to open source TRS for reading.\n");
        return -1;
    }
    
    Translation tra;
    HError err = ReadTRS(tra, in);
    delete in;
    if (!err)
    {
        printf("Error: failed to read source TRS:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write TRA
    //-----------------------------------------------------------------------//
    Stream *out = File::CreateFile(dst);
    if (!out)
    {
        printf("Error: failed to open output TRA for writing.\n");
        return -1;
    }
    tra.GameName = game_name;
    tra.GameUid = game_uid;
    err = WriteTRA(tra, out);
    delete out;
    if (!err)
    {
        printf("Error: failed to compile TRA:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    printf("Compiled translation written successfully.\nDone.\n");
    return 0;
}
