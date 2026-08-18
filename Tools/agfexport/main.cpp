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
#include "util/ini_util.h"
#include "util/stream.h"

using namespace AGS::Common;
using namespace AGS::DataUtil;
namespace AGF = AGS::AGF;

const char *HELP_STRING = ""
   //--------------------------------------------------------------------------------|
    "agfexport v0.3.0 - AGS game project miscellaneous export tool\n"
    "Copyright (c) 2026 AGS Team and contributors\n"
    "Usage: agfexport <COMMAND> [<OPTIONS>] <input-game.agf> <out-file>\n"
    "Commands:\n"
    "  audioclip-list         Exports list of audio clips and their sources\n"
    "  autoash                Generate auto script header\n"
    "  custom-data-dir        Exports list of custom data directories\n"
    "  font-list              Exports list of font files\n"
    "  game-cfg               Generate default game config\n"
    "  glvar                  Generate global variables scripts\n"
    "  header-list            Exports ordered list of headers from script modules\n"
    "  plugin-list            Exports list of game plugins\n"
    "  room-list              Exports list of active rooms\n"
    "  script-list            Exports ordered list of scripts from script modules\n"
    "  tra-list               Exports list of translations\n"
    "  -h, --help             Show help message for command\n";

const char *HELP_AUDIOCLIP_LIST = ""
    "Usage: agfexport audioclip-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with a list of audio files used by the game\n."
    "Each entry in the list consists of 3 comma-separated items:\n"
    "  * audio clip's import filename\n"
    "  * audio clip's source absolute filepath\n"
    "  * packing location, which is either 'ags' for the main game pack\n"
    "    or 'vox' for the separate 'audio.vox'\n"
#if (AGS_PLATFORM_OS_WINDOWS)
    "Entry example: au000001.ogg,C:\\Projects\\Audio\\OriginalMusic.ogg,vox\n"
#else
    "Entry example: au000001.ogg,~/Projects/Audio/OriginalMusic.ogg,vox\n"
#endif
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n"
    "  -s, --stdout           Instead print the list to stdout\n";

const char *HELP_AUTOASH = ""
    "Usage: agfexport autoash <INPUT-GAME.AGF> <OUT-FILE.ASH>\n"
    "Writes <OUT-FILE.ASH>, the auto-generated script header from <INPUT-GAME.AGF>\n"
    "This header has global elements from the game necessary for building scripts.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n";

const char *HELP_CUSTOMDATADIR_LIST = ""
    "Usage: agfexport custom-data-dir <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with a list of custom game data directories.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n"
    "  -s, --stdout           Instead print the list to stdout\n";

const char *HELP_FONT_LIST = ""
    "Usage: agfexport font-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with a list of font files used by the game\n."
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n"
    "  -s, --stdout           Instead print the list to stdout\n";

const char *HELP_GAMECFG = ""
    "Usage: agfexport game-cfg <INPUT-GAME.AGF> <OUT-FILE.CFG>\n"
    "Writes <OUT-FILE.CFG>, default game config.\n"
    "Config's contents are based on DefaultSetup node of the AGF file.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n";

const char *HELP_GLVAR = ""
    "Usage: agfexport glvar <INPUT-GAME.AGF> <HEAD.ASH> <BODY.ASC>\n"
    "Writes both <HEAD.ASH> (e.g. globalvars.ash) and <BODY.ASC> (e.g. globalvars.asc).\n"
    "These are retrieved from the game project in <INPUT-GAME.AGF>.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n";

const char *HELP_HEADER_LIST = ""
    "Usage: agfexport header-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with a list of headers from script modules.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n"
    "  -s, --stdout           Instead print the list to stdout\n";

const char *HELP_PLUGIN_LIST = ""
    "Usage: agfexport plugin-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with a list of game plugins.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n"
    "  -s, --stdout           Instead print the list to stdout\n";

const char *HELP_ROOM_LIST = ""
    "Usage: agfexport room-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with a list of rooms.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n"
    "  -s, --stdout           Instead print the list to stdout\n";

const char *HELP_SCRIPT_LIST = ""
    "Usage: agfexport script-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with an ordered list of scripts from script modules.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n"
    "  -s, --stdout           Instead print the list to stdout\n";

const char *HELP_TRA_LIST = ""
    "Usage: agfexport tra-list <INPUT-GAME.AGF> <OUT-FILE>\n"
    "Writes <OUT-FILE>, a file with a list of translations.\n"
    "Commands:\n"
    "  -h, --help             Show this help message\n"
    "  -n, --no-empty         Do not create any file if it's going to be empty\n"
    "  -s, --stdout           Instead print the list to stdout\n";

enum CommandType
{
    kCmdAudioClipList,
    kCmdAutoAsh,
    kCmdCustomDataDir,
    kCmdFontList,
    kCmdGameCfg,
    kCmdGlVar,
    kCmdHeaderList,
    kCmdPluginList,
    kCmdRoomList,
    kCmdScriptList,
    kCmdTraList,
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
        {"audioclip-list", kCmdAudioClipList, 2, HELP_AUDIOCLIP_LIST},
        {"autoash",     kCmdAutoAsh,    2, HELP_AUTOASH},
        {"custom-data-dir", kCmdCustomDataDir, 2, HELP_CUSTOMDATADIR_LIST},
        {"font-list",   kCmdFontList,   2, HELP_FONT_LIST},
        {"game-cfg",    kCmdGameCfg,    2, HELP_GAMECFG},
        {"glvar",       kCmdGlVar,      3, HELP_GLVAR},
        {"header-list", kCmdHeaderList, 2, HELP_HEADER_LIST},
        {"plugin-list", kCmdPluginList, 2, HELP_PLUGIN_LIST},
        {"room-list",   kCmdRoomList,   2, HELP_ROOM_LIST},
        {"script-list", kCmdScriptList, 2, HELP_SCRIPT_LIST},
        {"tra-list",    kCmdTraList,    2, HELP_TRA_LIST},
        {nullptr,       kCmdNone,       0, nullptr}
};

// A file target to print program log to (info, warnings and errors)
// TODO: replace this with a proper log system for tools.
FILE *StdFile = stdout;

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

HError write_to_file_if(const String &content, const String &file, bool skip_if_empty, const char *content_name)
{
    if (!skip_if_empty || !content.IsEmpty())
    {
        HError err = write_to_file(content, file);
        if (!err)
            return err;
        printf("%s written successfully\n", content_name);
    }
    else
    {
        printf("%s is empty, no writing done\n", content_name);
    }
    return HError::None();
}

HError list_command(const AGF::AGFReader &reader, CommandType cmd, const String &file, bool to_stdout, bool no_empty_files)
{
    if (!to_stdout)
        fprintf(StdFile, "Output list file: %s\n", file.GetCStr());

    String exp_data;

    if (cmd == kCmdAudioClipList)
    {
        std::vector<AudioClipData> clips;
        AGF::ReadAudioClips(clips, reader.GetGameRoot());
        for (const auto &clip : clips)
        {
            exp_data.AppendFmt("%s,%s,%s\n", clip.CacheFileName.GetCStr(), clip.SourceFileName.GetCStr(),
                clip.BundlingType == kAudioBundling_InMainData ? "ags" : "vox");
        }
    }

    if (cmd == kCmdCustomDataDir)
    {
        std::vector<String> dirs;
        AGF::ReadCustomDataDirectories(dirs, reader.GetGameRoot());
        for (const auto &dir : dirs)
        {
            exp_data.AppendFmt("%s\n", dir.GetCStr());
        }
    }

    if (cmd == kCmdFontList)
    {
        std::vector<int> font_index;
        AGF::ReadFontList(font_index, reader.GetGameRoot());
        // Unfortunately, in AGS 3.x project there's no explicit indication of a filename,
        // only font ID. The actual file is chosen at runtime among all variants, by certain priority rule.
        for (const auto &f : font_index)
        {
            exp_data.AppendFmt("agsfnt%d.ttf\n", f);
            exp_data.AppendFmt("agsfnt%d.wfn\n", f);
        }
    }

    if (cmd == kCmdHeaderList)
    {
        std::vector<String> scripts;
        AGF::ReadScriptHeaderList(scripts, reader.GetGameRoot());
        for (const auto &s: scripts)
            exp_data.AppendFmt("%s\n", s.GetCStr());
    }

    if (cmd == kCmdPluginList)
    {
        std::vector<String> plugins;
        AGF::ReadPluginList(plugins, reader.GetGameRoot());
        // We expect the plugin's filename to contain .dll extension
        for (const auto &s: plugins)
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

    if (cmd == kCmdScriptList)
    {
        std::vector<String> scripts;
        AGF::ReadScriptList(scripts, reader.GetGameRoot());
        for (const auto &s: scripts)
            exp_data.AppendFmt("%s\n", s.GetCStr());
    }

    if (cmd == kCmdTraList)
    {
        std::vector<String> translations;
        AGF::ReadTranslationList(translations, reader.GetGameRoot());
        for (const auto &s: translations)
            exp_data.AppendFmt("%s.trs\n", s.GetCStr());
    }

    if (to_stdout)
    {
        printf("%s", exp_data.GetCStr());
        return HError::None();
    }
    else
    {
        return write_to_file_if(exp_data, file, no_empty_files, "List");
    }
}

HError autoash_command(AGF::AGFReader &reader, const String &dst, bool no_empty_files)
{
    const char *dst_autoash = dst.GetCStr();
    fprintf(StdFile, "Output script header: %s\n", dst_autoash);

    GameRef game_ref;
    AGF::ReadGameRef(game_ref, reader);
    String header = MakeGameAutoScriptHeader(game_ref);
    return write_to_file_if(header, dst_autoash, no_empty_files, "Script header");
}

HError glvar_command(AGF::AGFReader &reader, const String &header_file, const String &body_file, bool no_empty_files)
{
    fprintf(StdFile, "Output script header: %s\n", header_file.GetCStr());
    fprintf(StdFile, "Output script body: %s\n", body_file.GetCStr());

    std::vector<Variable> vars;
    AGF::ReadGlobalVariables(vars, reader.GetGameRoot());
    String header = MakeVariablesScriptHeader(vars);
    String body = MakeVariablesScriptBody(vars);

    // Write both header and script if at least one of them is not empty
    if (!no_empty_files || !header.IsEmpty() || !body.IsEmpty())
    {
        auto err = write_to_file(header, header_file);
        if (!err)
            return err;
        printf("Script header written successfully\n");

        err = write_to_file(body, body_file);
        if (!err)
            return err;
        printf("Script body written successfully\n");
    }
    else
    {
        printf("Script is empty, no writing done\n");
    }

    return HError::None();
}

HError gamecfg_command(AGF::AGFReader &reader, const String &dst, bool no_empty_files)
{
    fprintf(StdFile, "Output config file: %s\n", dst.GetCStr());

    GameSettings settings;
    RuntimeSetup setup;
    AGF::ReadGameSettings(settings, reader.GetGameRoot());
    AGF::ReadRuntimeSetup(setup, reader.GetGameRoot());

    // Copy runtime setup to config tree
    ConfigTree cfg;
    WriteConfig(setup, &settings, cfg);

    // Write config tree as ini
    String str;
    IniUtil::WriteToString(str, cfg);
    return write_to_file(str, dst);
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

    const bool stdout_list_print = result.Opt.count("-s") || result.Opt.count("--stdout");
    const bool no_empty_files = result.Opt.count("-n") || result.Opt.count("--no-empty");
    if (stdout_list_print)
    {
        StdFile = stderr;
    }

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
                fprintf(StdFile, "%s\n", cmd_help);
                return 0;
            }
            if (asked_command_argc != required_cmd_argc)
            {
                fprintf(StdFile, "Error: required positional arguments don't match\n");
                fprintf(StdFile, "Requires %zu arguments, passed %zu\n", required_cmd_argc, asked_command_argc);
                fprintf(StdFile, "%s\n", cmd_help);
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
        fprintf(StdFile, "Error: unknown command '%s'\n", asked_command.GetCStr());
        fprintf(StdFile, "%s\n", HELP_STRING);
        return -1;
    }


    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//

    AGF::AGFReader reader;
    HError err = reader.Open(game_agf.GetCStr());
    if (!err)
    {
        fprintf(StdFile, "Error: failed to open source AGF '%s':\n", game_agf.GetCStr());
        fprintf(StdFile, "%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Execute command
    //-----------------------------------------------------------------------//
    String exp_data;
    switch (command)
    {
        case kCmdAudioClipList:
        case kCmdCustomDataDir:
        case kCmdFontList:
        case kCmdHeaderList:
        case kCmdPluginList:
        case kCmdRoomList:
        case kCmdScriptList:
        case kCmdTraList:
            err = list_command(reader, command, out_file, stdout_list_print, no_empty_files);
            break;
        case kCmdAutoAsh:
            err = autoash_command(reader, out_file, no_empty_files);
            break;
        case kCmdGlVar:
            err = glvar_command(reader, out_file, result.PosArgs[3], no_empty_files);
            break;
        case kCmdGameCfg:
            err = gamecfg_command(reader, out_file, no_empty_files);
            break;
        case kCmdMAX:
        default:
            // should never happen but handle just in case
            err = new Error("Internal error caused invalid command");
            break;
    }

    if (!err)
    {
        fprintf(StdFile, "Error: failed to execute command\n");
        fprintf(StdFile, "%s\n", err->FullMessage().GetCStr());
        return -1;
    }
    else if (!stdout_list_print)
    {
        fprintf(StdFile, "Data exported successfully.\n");
    }
    return 0;
}
