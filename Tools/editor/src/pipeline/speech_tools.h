// AGS Editor ImGui - Speech numbering and voice acting script tools
// Implements Auto-Number Speech Lines and Create Voice Acting Script
// following the same algorithm as the C# editor's TextProcessing classes.
#pragma once

#include <string>
#include <vector>
#include <map>

namespace AGSEditor
{

struct GameData;

// A single speech line entry
struct SpeechLineEntry
{
    int character_id = -1;
    std::string character_name;  // Script name (e.g. "cEgo") or "narrator"
    std::string text;            // The speech text (with &N prefix)
    std::string source_file;     // Which script file it came from
};

// Results from speech processing
struct SpeechProcessResult
{
    bool success = false;
    int lines_numbered = 0;
    int scripts_modified = 0;
    std::string speechref_path;
    std::string error_message;
    std::vector<std::string> warnings;
};

// Results from voice acting script generation
struct VoiceActingScriptResult
{
    bool success = false;
    std::string output_path;
    int total_lines = 0;
    int characters_found = 0;
    std::string error_message;
};

// Speech line numbering and voice acting tools
class SpeechTools
{
public:
    // Auto-number speech lines in all game scripts.
    // Scans for DisplaySpeech/Say/Think/Display calls, strips existing &N prefixes,
    // assigns new per-character sequential numbers, writes speechref.txt.
    // If include_narrator is true, also numbers Display() and DisplayAt() calls.
    // If combine_identical is true, identical lines for same character get same number.
    // If remove_numbering is true, strips &N prefixes without adding new ones.
    static SpeechProcessResult AutoNumberSpeechLines(
        const std::string& project_dir,
        GameData& game_data,
        bool include_narrator = false,
        bool combine_identical = false,
        bool remove_numbering = false);

    // Create a voice acting script by collecting all already-numbered &N lines
    // from game scripts and outputting them to a text file grouped by character.
    static VoiceActingScriptResult CreateVoiceActingScript(
        const std::string& project_dir,
        const GameData& game_data,
        const std::string& output_path);

private:
    // Represents a speech function call pattern
    enum class FunctionCallType
    {
        ObjectBased,    // cEgo.Say("..."), cEgo.Think("...")
        GlobalSpeech,   // DisplaySpeech(cEgo, "...")
        GlobalNarrator  // Display("..."), DisplayAt(...)
    };

    // Internal per-character counter for numbering
    struct CharacterCounter
    {
        int next_number = 1;
        std::map<std::string, std::string> seen_lines; // text -> "&N text" (for combining)
    };

    // Process a single script file for speech lines
    // If modify is true, rewrites the file with updated &N prefixes
    // If modify is false, just collects existing &N lines
    static void ProcessScriptFile(
        const std::string& filepath,
        const std::string& relative_name,
        const GameData& game_data,
        bool modify,
        bool include_narrator,
        bool combine_identical,
        bool remove_numbering,
        std::map<int, CharacterCounter>& counters,
        std::vector<SpeechLineEntry>& collected_lines,
        int& lines_processed,
        std::ofstream* speechref);

    // Find character ID from a script name (e.g. "cEgo" -> character ID)
    static int FindCharacterID(const std::string& name, const GameData& game_data);

    // Determine if a function call context matches a speech function
    static bool MatchesSpeechFunction(const std::string& context,
                                       int& out_char_id,
                                       const GameData& game_data,
                                       bool include_narrator);
};

} // namespace AGSEditor
