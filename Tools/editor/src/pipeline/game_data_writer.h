// AGS Editor ImGui - Game Data Writer (ac2game.dta)
// Writes game data in the exact binary format the AGS engine expects.
// This is a C++ reimplementation of C# DataFileWriter.SaveThisGameToFile().
#pragma once

#include <string>
#include <vector>

namespace AGS { namespace Common { class Stream; } }
namespace AGSEditor {
struct GameData;
struct BuildResult;

// Compiled script reference for writing into the game data file.
struct CompiledScriptRef {
    std::string name;      // Human-readable name
    std::string obj_file;  // Path to the .o compiled script file
};

// Write the complete ac2game.dta file from GameData.
// compiled_scripts: ordered list [global_script, dialog_script, module0, module1, ...]
// Returns true on success.
bool WriteGameDataFile(const GameData& gd,
                       const std::vector<CompiledScriptRef>& compiled_scripts,
                       const std::string& output_path,
                       BuildResult& result);

} // namespace AGSEditor
