#include <algorithm>
#include <stdio.h>
#include "data/agfreader.h"
#include "data/data_file_writer.h"
#include "data/xml_fields.h"
#include "util/cmdlineopts.h"
#include "util/file.h"
#include "util/path.h"
#include "util/stdio_compat.h"
#include "util/string_compat.h"
#include "util/string_utils.h"

using namespace AGS::Common;
using namespace AGS::Common::CmdLineOpts;
using namespace AGS;

const char *HELP_STRING =
//------------------------------------------------------------------------------|
"Usage: agf2dta <INPUT-GAME.AGF> <OUTPUT-FILE.DAT>\n"
"      Parses AGS project file INPUT-GAME.AGF and writes a compiled AGS game\n"
"      data file (v3.x-4.x engines expect 'game28.dta' by default).\n"
"Options:\n"
"  --script-dir <DIR>     embed the compiled scripts found in DIR (GlobalScript.o,\n"
"                         __DialogScripts.o and the modules), as the editor does;\n"
"                         otherwise the engine loads them from separate files\n"
"  --strict               treat elements the tool does not know, and values it\n"
"                         cannot read, as errors instead of warnings\n"
"  -h, --help             Show this help message\n";

// Loads a compiled script object, which goes into the game data as it is
static HError LoadCompiledScript(const String &script_dir, const String &script_name,
                             DataUtil::CompiledScriptData &script)
{
const String obj_file = Path::ConcatPaths(script_dir, Path::ReplaceExtension(script_name, "o"));
auto in = File::OpenFileRead(obj_file);
if (!in)
    return new Error(String::FromFormat("Compiled script not found: %s", obj_file.GetCStr()));
script.FileName = script_name;
script.Data.resize(static_cast<size_t>(in->GetLength()));
if (in->Read(script.Data.data(), script.Data.size()) != script.Data.size())
    return new Error(String::FromFormat("Failed to read %s", obj_file.GetCStr()));
return HError::None();
}

// Collects the compiled scripts as the editor embeds them (DataFileWriter.GetScriptModuleList):
// the global and dialog scripts, then the modules in compile order, the global variables first
static HError LoadCompiledScripts(const String &script_dir, AGF::AGFReader &reader, DataUtil::GameData &game)
{
const String global_script = "GlobalScript.asc";
std::vector<String> modules;
if (!game.GlobalVars.empty())
    modules.push_back("_GlobalVariables.asc");
std::vector<String> scripts;
AGF::ReadScriptList(scripts, reader.GetGameRoot());
for (const auto &script : scripts)
{
    if (script != global_script)
        modules.push_back(script);
}

HError err = LoadCompiledScript(script_dir, global_script, game.GlobalScript);
if (err)
    err = LoadCompiledScript(script_dir, "__DialogScripts.asc", game.DialogScript);
game.ScriptModules.resize(modules.size());
for (size_t i = 0; err && i < modules.size(); ++i)
    err = LoadCompiledScript(script_dir, modules[i], game.ScriptModules[i]);
game.EmbedScripts = static_cast<bool>(err);
return err;
}

int main(int argc, char *argv[])
{
printf("agf2dta v0.1.0 - AGS game main data file build tool\n"
       "Copyright (c) 2026 AGS Team and contributors\n");
ParseResult parseResult = Parse(argc, argv, {"--script-dir"});
    if (parseResult.HelpRequested)
    {
        printf("%s\n", HELP_STRING);
        return 0; // display help and bail out
    }
    if (parseResult.PosArgs.size() < 2)
    {
        printf("Error: not enough arguments\n");
        printf("%s\n", HELP_STRING);
        return -1;
    }
	
    const String &src_agf = parseResult.PosArgs[0];
    const String &out_file = parseResult.PosArgs[1];
	
    printf("Input game AGF: %s\n", src_agf.GetCStr());
    printf("Output dat file: %s\n", out_file.GetCStr());
	
    //-----------------------------------------------------------------------//
    // validate inputs
    //-----------------------------------------------------------------------//
    if (!File::IsFile(src_agf))
    {
        printf("Error: input game file '%s' not found.\n", src_agf.GetCStr());
        return -1;
    }

    //-----------------------------------------------------------------------//
    // Read Game.agf
    //-----------------------------------------------------------------------//
    AGF::AGFReader reader;
    const HError err = reader.Open(src_agf.GetCStr());
    if (!err)
    {
        printf("Error: failed to open source AGF:\n");
        printf("%s\n", err->FullMessage().GetCStr());
        return -1;
    }

    DataUtil::GameData game{};
    AGF::XmlProblems::Clear();
    AGF::ReadGameData(game, reader);
    const bool strict = parseResult.Opt.count("--strict") > 0;
    for (const String &problem : AGF::XmlProblems::Get())
        printf("%s: %s\n", strict ? "Error" : "Warning", problem.GetCStr());
    if (strict && !AGF::XmlProblems::Get().empty())
        return -1;
    for (const auto &opt : parseResult.OptWithValue)
    {
        if (opt.first != "--script-dir")
            continue;
        const HError script_err = LoadCompiledScripts(opt.second, reader, game);
        if (!script_err)
        {
            printf("Error: %s\n", script_err->FullMessage().GetCStr());
            return -1;
        }
    }

    //-----------------------------------------------------------------------//
    // Set the main game data struct values from Game.agf
    //-----------------------------------------------------------------------//

    // TODO: Refactor to use this approach instead, for now we are just replicating DataFileWriter.cs approach instead

    //-----------------------------------------------------------------------//
    // Write main game data file to game28.dta
    //-----------------------------------------------------------------------//
    std::unique_ptr<Stream> out = File::CreateFile(out_file);
    if (!out)
    {
        printf("Error: unable to create output file '%s'.\n", out_file.GetCStr());
        return -1;
    }

    HError write_err = DataUtil::WriteGameData28(game, std::move(out));
    if (!write_err)
    {
        printf("Error: failed to write game data:\n");
        printf("%s\n", write_err->FullMessage().GetCStr());
        return -1;
    }

    printf("Game main data file(s) written successfully.\nDone.\n");
    return 0;
}
