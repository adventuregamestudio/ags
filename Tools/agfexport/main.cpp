#include <memory>
#include <stdio.h>
#include <string.h>
#include "data/agfreader.h"
#include "util/file.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;
namespace AGF = AGS::AGF;


enum Command
{
    kCmdNone,
    kCmdScriptList,
    kCmdRoomList
};

struct CommandType
{
    const char *Opt;
    Command     Cmd;
    int         NumArgs;
} CommandTypes[] = {
    { "--script-list"    , kCmdScriptList, 0 },
    { "--room-list"      , kCmdRoomList, 0 },
    { nullptr            , kCmdNone, 0 }
};

const char *HELP_STRING = "Usage: agfexport <input-game.agf> <COMMAND> [<OPTIONS>] <out-file>\n"
"Commands:\n"
"  --script-list <file>   exports ordered list of script modules\n"
"  --room-list <file>     exports list of active rooms\n";

int main(int argc, char *argv[])
{
    printf("agfexport v0.1.0 - AGS game's miscellaneous export tool\n"\
        "Copyright (c) 2021 AGS Team and contributors\n");
    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        if (strcmp(arg, "--help") == 0 || strcmp(arg, "/?") == 0 || strcmp(arg, "-?") == 0)
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
    const char *command = argv[2];
    Command cmd_type = kCmdNone;
    int cmd_arg_num = 0;
    for (size_t i = 0; CommandTypes[i].Opt; ++i)
    {
        if (strcmp(command, CommandTypes[i].Opt) == 0)
        {
            cmd_type = CommandTypes[i].Cmd;
            cmd_arg_num = CommandTypes[i].NumArgs;
            break;
        }
    }
    if (cmd_type == kCmdNone)
    {
        printf("Error: unknown command '%s'\n", command);
        printf("%s\n", HELP_STRING);
        return -1;
    }

    const char *dst = argv[3 + cmd_arg_num];
    printf("Input game AGF: %s\n", src);
    printf("Output file: %s\n", dst);

    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//
    AGF::AGFReader reader;
    HError err = reader.Open(src);
    if (!err)
    {
        printf("Error: failed to open source AGF:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Prepare export
    //-----------------------------------------------------------------------//
    String exp_data;
    switch (cmd_type)
    {
    case kCmdScriptList:
        {
            std::vector<String> scripts;
            AGF::ReadScriptList(scripts, reader.GetGameRoot());
            for (const auto &s : scripts)
                exp_data.AppendFmt("%s\n", s.GetCStr());
        }
        break;
    case kCmdRoomList:
        {
            std::vector<std::pair<int, String>> rooms;
            AGF::ReadRoomList(rooms, reader.GetGameRoot());
            for (const auto &r : rooms)
                exp_data.AppendFmt("room%d.crm:%s\n", r.first, r.second.GetCStr());
        }
        break;
    }

    //-----------------------------------------------------------------------//
    // Write script header
    //-----------------------------------------------------------------------//
    std::unique_ptr<Stream> out( File::CreateFile(dst) );
    if (!out)
    {
        printf("Error: failed to open an output file for writing.\n");
        return -1;
    }
    out->Write(exp_data.GetCStr(), exp_data.GetLength());
    printf("Data exported successfully.\nDone.\n");
    return 0;
}
