// AGS Editor ImGui - Speech numbering and voice acting script tools
#include "speech_tools.h"
#include "project/game_data.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <set>
#include <filesystem>
#include <cstdio>
#include <regex>

namespace fs = std::filesystem;

namespace AGSEditor
{

// Special character ID for narrator (matches AGS convention)
static const int NARRATOR_CHARACTER_ID = 999;

int SpeechTools::FindCharacterID(const std::string& name, const GameData& game_data)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    if (lower == "narrator") return NARRATOR_CHARACTER_ID;
    if (lower == "player")
    {
        // Return player character ID
        if (game_data.player_character_id >= 0 &&
            game_data.player_character_id < (int)game_data.characters.size())
            return game_data.characters[game_data.player_character_id].id;
        return -1;
    }

    // Match by script name (case-insensitive)
    for (const auto& ch : game_data.characters)
    {
        std::string sn = ch.script_name;
        std::transform(sn.begin(), sn.end(), sn.begin(), ::tolower);
        if (sn == lower) return ch.id;
        // Also try without the leading 'c' prefix
        if (sn.size() > 1 && sn[0] == 'c')
        {
            if (sn.substr(1) == lower) return ch.id;
        }
    }
    // Try adding 'c' prefix
    std::string with_c = "c" + lower;
    for (const auto& ch : game_data.characters)
    {
        std::string sn = ch.script_name;
        std::transform(sn.begin(), sn.end(), sn.begin(), ::tolower);
        if (sn == with_c) return ch.id;
    }
    return -1;
}

bool SpeechTools::MatchesSpeechFunction(const std::string& context,
                                         int& out_char_id,
                                         const GameData& game_data,
                                         bool include_narrator)
{
    // Check for object-based calls: cEgo.Say(, cEgo.Think(
    {
        // Look for ".Say(" or ".Think("
        auto check_method = [&](const std::string& method) -> bool {
            size_t pos = context.find(method);
            if (pos == std::string::npos || pos == 0) return false;

            // Extract the object name before the dot
            size_t dot_pos = pos;
            // Scan backwards to find the start of the identifier
            int start = (int)dot_pos - 1;
            while (start >= 0 && (std::isalnum((unsigned char)context[start]) || context[start] == '_'))
                start--;
            start++;
            if (start >= (int)dot_pos) return false;

            std::string obj_name = context.substr(start, dot_pos - start);
            out_char_id = FindCharacterID(obj_name, game_data);
            return out_char_id >= 0;
        };

        if (check_method(".Say(") || check_method(".SayAt(") ||
            check_method(".Think(") || check_method(".SayBackground("))
            return true;
    }

    // Check for global speech functions: DisplaySpeech(cEgo, ...), DisplayThought(cEgo, ...)
    {
        auto check_global = [&](const std::string& func) -> bool {
            size_t pos = context.find(func);
            if (pos == std::string::npos) return false;

            // Find the first argument (character name or ID)
            size_t paren = context.find('(', pos);
            if (paren == std::string::npos) return false;

            // Skip whitespace after '('
            size_t arg_start = paren + 1;
            while (arg_start < context.size() && std::isspace((unsigned char)context[arg_start]))
                arg_start++;

            // Read the argument until ',' or ')'
            size_t arg_end = arg_start;
            while (arg_end < context.size() && context[arg_end] != ',' && context[arg_end] != ')')
                arg_end++;

            std::string arg = context.substr(arg_start, arg_end - arg_start);
            // Trim whitespace
            while (!arg.empty() && std::isspace((unsigned char)arg.back())) arg.pop_back();
            while (!arg.empty() && std::isspace((unsigned char)arg.front())) arg.erase(arg.begin());

            if (arg.empty()) return false;

            // Could be a numeric ID or a character script name
            if (std::isdigit((unsigned char)arg[0]))
            {
                try { out_char_id = std::stoi(arg); return true; }
                catch (...) { return false; }
            }

            out_char_id = FindCharacterID(arg, game_data);
            return out_char_id >= 0;
        };

        if (check_global("DisplaySpeech") || check_global("DisplayThought"))
            return true;
    }

    // Narrator functions (only if include_narrator)
    if (include_narrator)
    {
        if (context.find("Display(") != std::string::npos ||
            context.find("DisplayAt(") != std::string::npos ||
            context.find("DisplayAtY(") != std::string::npos)
        {
            out_char_id = NARRATOR_CHARACTER_ID;
            return true;
        }
    }

    return false;
}

void SpeechTools::ProcessScriptFile(
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
    std::ofstream* speechref)
{
    std::ifstream file(filepath);
    if (!file.is_open()) return;

    std::string script((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();

    bool modified = false;
    std::string result;
    result.reserve(script.size() + 256);

    size_t i = 0;
    while (i < script.size())
    {
        // Skip single-line comments
        if (i + 1 < script.size() && script[i] == '/' && script[i + 1] == '/')
        {
            size_t end = script.find('\n', i);
            if (end == std::string::npos) end = script.size();
            else end++;
            result.append(script, i, end - i);
            i = end;
            continue;
        }

        // Skip block comments
        if (i + 1 < script.size() && script[i] == '/' && script[i + 1] == '*')
        {
            size_t end = script.find("*/", i + 2);
            if (end == std::string::npos) end = script.size();
            else end += 2;
            result.append(script, i, end - i);
            i = end;
            continue;
        }

        // Skip single-quoted chars
        if (script[i] == '\'')
        {
            result += script[i++];
            while (i < script.size() && script[i] != '\'')
            {
                if (script[i] == '\\' && i + 1 < script.size())
                {
                    result += script[i++];
                }
                result += script[i++];
            }
            if (i < script.size()) result += script[i++]; // closing quote
            continue;
        }

        // Found a string literal
        if (script[i] == '"')
        {
            size_t string_start = i;
            i++; // skip opening quote
            size_t text_start = i;

            // Find closing quote (handle escapes)
            while (i < script.size() && script[i] != '"')
            {
                if (script[i] == '\\' && i + 1 < script.size())
                    i++; // skip escaped char
                i++;
            }

            std::string text_content = script.substr(text_start, i - text_start);
            size_t string_end = i; // position of closing quote

            if (i < script.size()) i++; // skip closing quote

            // Look back to find the function call context
            // Grab up to 200 chars before the string start
            size_t ctx_start = (string_start > 200) ? string_start - 200 : 0;
            std::string context = script.substr(ctx_start, string_start - ctx_start);

            int char_id = -1;
            bool is_speech = MatchesSpeechFunction(context, char_id, game_data, include_narrator);

            if (is_speech && !text_content.empty() && text_content != "...")
            {
                if (modify)
                {
                    // Strip existing &N prefix
                    std::string clean_text = text_content;
                    if (!clean_text.empty() && clean_text[0] == '&')
                    {
                        size_t j = 1;
                        while (j < clean_text.size() && std::isdigit((unsigned char)clean_text[j]))
                            j++;
                        // Skip space after number
                        while (j < clean_text.size() && clean_text[j] == ' ')
                            j++;
                        clean_text = clean_text.substr(j);
                    }

                    std::string new_text;
                    if (remove_numbering)
                    {
                        new_text = clean_text;
                    }
                    else
                    {
                        // Check for combining identical lines
                        if (combine_identical)
                        {
                            auto& counter = counters[char_id];
                            auto it = counter.seen_lines.find(clean_text);
                            if (it != counter.seen_lines.end())
                            {
                                new_text = it->second;
                            }
                            else
                            {
                                int num = counter.next_number++;
                                new_text = "&" + std::to_string(num) + " " + clean_text;
                                counter.seen_lines[clean_text] = new_text;
                            }
                        }
                        else
                        {
                            auto& counter = counters[char_id];
                            int num = counter.next_number++;
                            new_text = "&" + std::to_string(num) + " " + clean_text;

                            if (combine_identical)
                                counter.seen_lines[clean_text] = new_text;
                        }
                    }

                    if (new_text != text_content)
                        modified = true;

                    result += '"';
                    result += new_text;
                    result += '"';

                    // Write to speechref
                    if (speechref && !remove_numbering)
                    {
                        std::string char_name;
                        if (char_id == NARRATOR_CHARACTER_ID)
                            char_name = "narrator";
                        else
                        {
                            for (const auto& ch : game_data.characters)
                            {
                                if (ch.id == char_id)
                                {
                                    char_name = ch.script_name;
                                    break;
                                }
                            }
                            if (char_name.empty())
                                char_name = "char" + std::to_string(char_id);
                        }
                        *speechref << char_name << ": " << new_text << "\n";
                    }

                    lines_processed++;
                }
                else
                {
                    // Read-only: collect lines that already have &N prefix
                    if (!text_content.empty() && text_content[0] == '&')
                    {
                        SpeechLineEntry entry;
                        entry.character_id = char_id;
                        if (char_id == NARRATOR_CHARACTER_ID)
                            entry.character_name = "narrator";
                        else
                        {
                            for (const auto& ch : game_data.characters)
                            {
                                if (ch.id == char_id)
                                {
                                    entry.character_name = ch.script_name;
                                    break;
                                }
                            }
                            if (entry.character_name.empty())
                                entry.character_name = "char" + std::to_string(char_id);
                        }
                        entry.text = text_content;
                        entry.source_file = relative_name;
                        collected_lines.push_back(std::move(entry));
                        lines_processed++;
                    }
                    // Don't modify - just pass through
                    result += '"';
                    result += text_content;
                    result += '"';
                }
            }
            else
            {
                // Not a speech function call, pass through
                result += '"';
                result += text_content;
                result += '"';
            }
            continue;
        }

        // Regular character
        result += script[i++];
    }

    // Write back if modified
    if (modify && modified)
    {
        std::ofstream out(filepath, std::ios::trunc);
        if (out.is_open())
        {
            out << result;
            out.close();
        }
    }
}

SpeechProcessResult SpeechTools::AutoNumberSpeechLines(
    const std::string& project_dir,
    GameData& game_data,
    bool include_narrator,
    bool combine_identical,
    bool remove_numbering)
{
    SpeechProcessResult result;
    result.speechref_path = project_dir + "/speechref.txt";

    std::ofstream speechref(result.speechref_path);
    if (!speechref.is_open())
    {
        result.error_message = "Cannot create speechref.txt in project directory.";
        return result;
    }
    speechref << "// AGS auto-numbered speech lines output. This file was automatically generated.\n";

    std::map<int, CharacterCounter> counters;
    std::vector<SpeechLineEntry> dummy_collected;
    int total_lines = 0;
    int scripts_modified = 0;

    // Process all script files (.asc files)
    // 1. GlobalScript.asc first
    std::string global_script = project_dir + "/GlobalScript.asc";
    if (fs::exists(global_script))
    {
        int before = total_lines;
        ProcessScriptFile(global_script, "GlobalScript.asc", game_data,
            true, include_narrator, combine_identical, remove_numbering,
            counters, dummy_collected, total_lines, &speechref);
        if (total_lines > before) scripts_modified++;
    }

    // 2. Script modules
    for (const auto& mod : game_data.script_modules)
    {
        if (!mod.script_file.empty())
        {
            std::string path = project_dir + "/" + mod.script_file;
            if (fs::exists(path))
            {
                int before = total_lines;
                ProcessScriptFile(path, mod.script_file, game_data,
                    true, include_narrator, combine_identical, remove_numbering,
                    counters, dummy_collected, total_lines, &speechref);
                if (total_lines > before) scripts_modified++;
            }
        }
    }

    // 3. Room scripts (roomN.asc)
    for (const auto& room : game_data.rooms)
    {
        char room_script[64];
        snprintf(room_script, sizeof(room_script), "room%d.asc", room.number);
        std::string path = project_dir + "/" + room_script;
        if (fs::exists(path))
        {
            int before = total_lines;
            ProcessScriptFile(path, room_script, game_data,
                true, include_narrator, combine_identical, remove_numbering,
                counters, dummy_collected, total_lines, &speechref);
            if (total_lines > before) scripts_modified++;
        }
    }

    speechref.close();

    result.success = true;
    result.lines_numbered = total_lines;
    result.scripts_modified = scripts_modified;
    return result;
}

VoiceActingScriptResult SpeechTools::CreateVoiceActingScript(
    const std::string& project_dir,
    const GameData& game_data,
    const std::string& output_path)
{
    VoiceActingScriptResult result;
    result.output_path = output_path;

    std::map<int, CharacterCounter> counters; // unused for read-only
    std::vector<SpeechLineEntry> collected;
    int total_lines = 0;

    // Process all scripts in read-only mode (modify=false)
    // Only collects lines that already have &N prefixes

    // 1. GlobalScript.asc
    std::string global_script = project_dir + "/GlobalScript.asc";
    if (fs::exists(global_script))
    {
        ProcessScriptFile(global_script, "GlobalScript.asc", game_data,
            false, true, false, false,
            counters, collected, total_lines, nullptr);
    }

    // 2. Script modules
    for (const auto& mod : game_data.script_modules)
    {
        if (!mod.script_file.empty())
        {
            std::string path = project_dir + "/" + mod.script_file;
            if (fs::exists(path))
            {
                ProcessScriptFile(path, mod.script_file, game_data,
                    false, true, false, false,
                    counters, collected, total_lines, nullptr);
            }
        }
    }

    // 3. Room scripts
    for (const auto& room : game_data.rooms)
    {
        char room_script[64];
        snprintf(room_script, sizeof(room_script), "room%d.asc", room.number);
        std::string path = project_dir + "/" + room_script;
        if (fs::exists(path))
        {
            ProcessScriptFile(path, room_script, game_data,
                false, true, false, false,
                counters, collected, total_lines, nullptr);
        }
    }

    if (collected.empty())
    {
        result.error_message = "No speech-numbered lines (&N) found in scripts. "
            "Run Auto-Number Speech Lines first.";
        return result;
    }

    // Write voice acting script
    std::ofstream out(output_path);
    if (!out.is_open())
    {
        result.error_message = "Cannot create output file: " + output_path;
        return result;
    }

    out << "// AGS voice acting script. This file was automatically generated.\n";
    out << "// Total speech lines: " << collected.size() << "\n\n";

    // Section 1: Group by character
    std::map<int, std::vector<const SpeechLineEntry*>> by_character;
    for (const auto& entry : collected)
        by_character[entry.character_id].push_back(&entry);

    std::set<int> char_ids;
    for (const auto& pair : by_character)
        char_ids.insert(pair.first);

    for (int cid : char_ids)
    {
        const auto& lines = by_character[cid];
        std::string char_display;
        if (cid == NARRATOR_CHARACTER_ID)
            char_display = "Narrator";
        else
        {
            for (const auto& ch : game_data.characters)
            {
                if (ch.id == cid)
                {
                    char_display = ch.real_name.empty() ? ch.script_name : ch.real_name;
                    char_display += " (" + ch.script_name + ")";
                    break;
                }
            }
            if (char_display.empty())
                char_display = "Character " + std::to_string(cid);
        }

        out << "*** Lines for " << char_display << " ***\n\n";
        for (const auto* entry : lines)
        {
            out << "  " << entry->text << "\n";
        }
        out << "\n";
    }

    // Section 2: All lines in order of appearance
    out << "\n*** All text lines, in order of appearance in the scripts ***\n\n";
    for (const auto& entry : collected)
    {
        out << entry.character_name << ": " << entry.text << "\n";
    }

    out.close();

    result.success = true;
    result.total_lines = (int)collected.size();
    result.characters_found = (int)by_character.size();
    return result;
}

} // namespace AGSEditor
