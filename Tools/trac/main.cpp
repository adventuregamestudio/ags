#include <stdio.h>
#include "data/tra_utils.h"
#include "util/file.h"
#include "util/path.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;


//------------------------------------------------------------------------------|
const char *HELP_STRING = "Usage:\n"
"  trac <input.trs> [<output.tra>] [--gamename <name>][--uniqueid <idnum>]\n"
"  trac -u <input.tra> [<output.trs>]\n";

int Command_Compile(const String &src, const String &dst, const String &game_name, int game_uid)
{
    printf("Input translation source: %s\n", src.GetCStr());
    printf("Output compiled translation: %s\n", dst.GetCStr());
    printf("Game name: %s\n", game_name.GetCStr());
    printf("Game uniqueid: %d\n", game_uid);

    //-----------------------------------------------------------------------//
    // Read TRS
    //-----------------------------------------------------------------------//
    auto in = File::OpenFileRead(src);
    if (!in)
    {
        printf("Error: failed to open source TRS for reading.\n");
        return -1;
    }

    Translation tra;
    HError err = ReadTRS(tra, std::move(in));
    if (!err)
    {
        printf("Error: failed to read source TRS:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write TRA
    //-----------------------------------------------------------------------//
    auto out = File::CreateFile(dst);
    if (!out)
    {
        printf("Error: failed to open output TRA for writing.\n");
        return -1;
    }
    tra.GameName = game_name;
    tra.GameUid = game_uid;
    err = WriteTRA(tra, std::move(out));
    if (!err)
    {
        printf("Error: failed to compile TRA:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    printf("Compiled translation written successfully.\nDone.\n");
    return 0;
}

int Command_Decompile(const String &src, const String &dst)
{
    printf("Input compiled translation: %s\n", src.GetCStr());
    printf("Output translation source: %s\n", dst.GetCStr());

    //-----------------------------------------------------------------------//
    // Read TRA
    //-----------------------------------------------------------------------//
    auto in = File::OpenFileRead(src);
    if (!in)
    {
        printf("Error: failed to open TRA for reading.\n");
        return -1;
    }

    Translation tra;
    HError err = ReadTraData(tra, std::move(in));
    if (!err)
    {
        printf("Error: failed to read input TRA:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Write TRS
    //-----------------------------------------------------------------------//
    auto out = File::CreateFile(dst);
    if (!out)
    {
        printf("Error: failed to open output TRS for writing.\n");
        return -1;
    }
    err = WriteTRS(tra, std::move(out));
    if (!err)
    {
        printf("Error: failed to write TRS:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    printf("Translation source written successfully.\nDone.\n");
    return 0;
}

int main(int argc, char *argv[])
{
    printf("trac v0.9.5 - AGS translation compiler and decompiler (TRS <-> TRA)\n"\
        "Copyright (c) 2021-2026 AGS Team and contributors\n");
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (ags_stricmp(arg, "--help") == 0 || ags_stricmp(arg, "/?") == 0 || ags_stricmp(arg, "-?") == 0)
        {
            printf("%s\n", HELP_STRING);
            return 0; // display help and bail out
        }
    }

    if (argc < 2)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }

    // TODO: redo this using command args utility
    if (argv[1][0] != '-')
    {
        String src = argv[1];
        String dst;
        String game_name;
        int game_uid = 0;
        for (int i = 2; i < argc; ++i)
        {
            const char *arg = argv[i];
            if (arg[0] != '-' && dst.IsEmpty())
                dst = arg;
            else if (ags_stricmp(arg, "--gamename") == 0 && (i < argc - 1))
                game_name = argv[++i];
            else if (ags_stricmp(arg, "--uniqueid") == 0 && (i < argc - 1))
                game_uid = StrUtil::StringToInt(argv[++i]);
        }

        if (dst.IsEmpty())
            dst = Path::ReplaceExtension(src, "tra");

        return Command_Compile(src, dst, game_name, game_uid);
    }
    else if (ags_stricmp(argv[1], "-u") == 0)
    {
        if (argc < 3)
        {
            printf("Error: not enough arguments\n");
            printf("%s\n", HELP_STRING);
            return -1;
        }

        String src = argv[2];
        String dst = argv[3];
        if (dst.IsEmpty())
            dst = Path::ReplaceExtension(src, "trs");

        return Command_Decompile(src, dst);
    }
    else
    {
        printf("Error: incorrect command syntax\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }
}
