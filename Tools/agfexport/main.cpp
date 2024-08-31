#include <memory>
#include <set>
#include <vector>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <util/cmdlineopts.h>
#include "data/agfreader.h"
#include "data/scriptgen.h"
#include "util/file.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;
namespace AGF = AGS::AGF;

const char *HELP_STRING = ""
    "agfexport v0.2.0 - AGS game project miscellaneous export tool\n"
    "Copyright (c) 2024 AGS Team and contributors\n"
    "Usage: agfexport <COMMAND> [<OPTIONS>] <input-game.agf> <out-file>\n"
    "Commands:\n"
    "  autoash                Generate auto script header\n"
    "  glvar                  Generate global variables scripts\n"
    "  header-list            Exports ordered list of headers from script modules\n"
    "  script-list            Exports ordered list of scripts from script modules\n"
    "  room-list              Exports list of active rooms\n"
    "  -h, --help             Show help message for command\n";

const char *HELP_AUTOASH = ""
    "Usage: agfexport autoash <INPUT-GAME.AGF> <OUT-FILE.ASH>\n"
    "Writes <OUT-FILE.ASH>, the auto-generated script header from <INPUT-GAME.AGF>\n"
    "This header has global elements from the game necessary for building scripts.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n";

const char *HELP_GLVAR = ""
    "Usage: agfexport glvar <INPUT-GAME.AGF> <HEAD.ASH> <BODY.ASC>\n"
    "Writes both <HEAD.ASH> (e.g. globalvars.ash) and <BODY.ASC> (e.g. globalvars.asc).\n"
    "These are retrieved from the game project in <INPUT-GAME.AGF>.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n";

const char *HELP_HEADER_LIST = ""
    "Usage: agfexport header-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with a list of headers from script modules."
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -t, --to-stdout        Instead write the list of scripts to stdout";

const char *HELP_SCRIPT_LIST = ""
    "Usage: agfexport script-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with an ordered list of scripts from script modules."
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -t, --to-stdout        Instead write the list of scripts to stdout";

const char *HELP_ROOM_LIST = ""
    "Usage: agfexport room-list <INPUT-GAME.AGF> <OUT-FILE>\n"
     "Writes <OUT-FILE>, a file with a list of rooms."
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -t, --to-stdout        Instead write the list of rooms to stdout";

enum CommandType
{
    kCmdAutoAsh = 0,
    kCmdGlVar,
    kCmdHeaderList,
    kCmdScriptList,
    kCmdRoomList,
    kCmdMAX,
    kCmdNone = kCmdMAX
};

struct Command
{
    const char *Opt;
    const CommandType Cmd;
    const size_t NumArgs;
    const char *Help;
} Command[] = {
        {"autoash",     kCmdAutoAsh,    2, HELP_AUTOASH},
        {"glvar",       kCmdGlVar,      3, HELP_GLVAR},
        {"header-list", kCmdHeaderList, 2, HELP_HEADER_LIST},
        {"script-list", kCmdScriptList, 2, HELP_SCRIPT_LIST},
        {"room-list",   kCmdRoomList,   2, HELP_ROOM_LIST},
        {nullptr,       kCmdNone,       0, nullptr}
};

HError write_to_file(const String &content, const String &file)
{
    std::unique_ptr<Stream> out(File::CreateFile(file));
    if (!out)
    {

        return new Error(String::FromFormat("Failed to open output file '%s' for writing.", file.GetCStr()));
    }
    out->Write(content.GetCStr(), content.GetLength());
    return HError::None();
}

HError list_command(const AGF::AGFReader &reader, CommandType cmd, const String &file, bool to_stdout)
{
    String exp_data;

    if (cmd == kCmdScriptList)
    {
        std::vector<String> scripts;
        AGF::ReadScriptList(scripts, reader.GetGameRoot());
        for (const auto &s: scripts)
            exp_data.AppendFmt("%s\n", s.GetCStr());
    }

    if (cmd == kCmdHeaderList)
    {
        std::vector<String> scripts;
        AGF::ReadScriptHeaderList(scripts, reader.GetGameRoot());
        for (const auto &s: scripts)
            exp_data.AppendFmt("%s\n", s.GetCStr());
    }

    if (cmd == kCmdRoomList)
    {
        std::vector<int> rooms;
        std::vector<std::pair<int, String>> rooms_dsc;
        AGF::ReadRoomList(rooms_dsc, reader.GetGameRoot());
        rooms.reserve(rooms_dsc.size());
        for (const auto &rd: rooms_dsc)
                    rooms.push_back(rd.first);

        std::sort(rooms.begin(), rooms.end());
        for (const auto &r: rooms)
            exp_data.AppendFmt("room%d.crm\n", r);
    }

    if (to_stdout)
    {
        printf("%s", exp_data.GetCStr());
        return HError::None();
    }

    return write_to_file(exp_data, file);
}

HError autoash_command(AGF::AGFReader &reader, const String &dst)
{
    const char *dst_autoash = dst.GetCStr();
    printf("Output script header: %s\n", dst_autoash);

    GameRef game_ref;
    AGF::ReadGameRef(game_ref, reader);
    String header = MakeGameAutoScriptHeader(game_ref);
    return write_to_file(header, dst_autoash);
}

HError glvar_command(AGF::AGFReader &reader, const String &header_file, const String &body_file)
{
    printf("Output script header: %s\n", header_file.GetCStr());
    printf("Output script body: %s\n", body_file.GetCStr());

    std::vector<Variable> vars;
    AGF::ReadGlobalVariables(vars, reader.GetGameRoot());
    String header = MakeVariablesScriptHeader(vars);
    String body = MakeVariablesScriptBody(vars);

    auto err = write_to_file(header, header_file);
    if (!err)
        return err;

    printf("Script header written successfully.\n");

    err = write_to_file(body, body_file);
    if (!err)
        return err;

    printf("Script body written successfully.\n");

    return HError::None();
}


int main(int argc, char *argv[])
{
    std::set<String> options_with_values;

    auto result = CmdLineOpts::Parse(argc, argv, options_with_values);

    if (result.PosArgs.empty())
    {
        printf("%s\n", HELP_STRING);
        return result.HelpRequested ? 0 : -1;
    }

    const bool stdout_list_print = result.Opt.count("-t") || result.Opt.count("--to-stdout");
    // for (auto &owv: result.OptWithValue)
    // {
    //    const String &opt = owv.first;
    //    const String &value = owv.second;
    // }

    //-----------------------------------------------------------------------//
    // Parse command specific arguments
    //-----------------------------------------------------------------------//

    const String &asked_command = result.PosArgs[0];
    const size_t asked_command_argc = result.PosArgs.size() - 1;
    CommandType command = kCmdNone;
    String out_file = nullptr;
    String game_agf = nullptr;

    for (int cmd = 0; cmd < kCmdMAX; cmd++)
    {
        if (asked_command.Equals(Command[cmd].Opt))
        {
            command = static_cast<CommandType>(cmd);
            const size_t required_cmd_argc = Command[cmd].NumArgs - (stdout_list_print ? 1 : 0);
            const char *cmd_help = Command[cmd].Help;
            if (result.HelpRequested)
            {
                printf("%s\n", cmd_help);
                return 0;
            }
            if (asked_command_argc != required_cmd_argc)
            {
                printf("Error: required positional arguments don't match\n");
                printf("Requires %zu arguments, passed %zu\n", required_cmd_argc, asked_command_argc);
                printf("%s\n", cmd_help);
                return -1;
            }

            game_agf = result.PosArgs[1];
            if(!stdout_list_print)
            {
                out_file = result.PosArgs[2];
            }
        }
    }

    if (command == kCmdNone)
    {
        printf("Error: unknown command '%s'\n", asked_command.GetCStr());
        printf("%s\n", HELP_STRING);
        return -1;
    }


    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//

    AGF::AGFReader reader;
    HError err = reader.Open(game_agf.GetCStr());
    if (!err)
    {
        printf("Error: failed to open source AGF '%s':\n", game_agf.GetCStr());
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Execute command
    //-----------------------------------------------------------------------//
    String exp_data;
    switch (command)
    {
        case kCmdHeaderList:
        case kCmdScriptList:
        case kCmdRoomList:
            err = list_command(reader, command, out_file, stdout_list_print);
            break;
        case kCmdAutoAsh:
            err = autoash_command(reader, out_file);
            break;
        case kCmdGlVar:
            err = glvar_command(reader, out_file, result.PosArgs[3]);
            break;
        case kCmdMAX:
        default:
            // should never happen but handle just in case
            err = new Error("Internal error caused invalid command");
            break;
    }

    if (!err)
    {
        printf("Error: failed to execute command\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    } else if(!stdout_list_print) {
        printf("Data exported successfully.\n");
    }
    return 0;
}
