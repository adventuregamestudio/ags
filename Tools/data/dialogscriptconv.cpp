//=============================================================================
//
// Adventure Game Studio (AGS)
//
// Copyright (C) 1999-2011 Chris Jones and 2011-2023 various contributors
// The full list of copyright holders can be found in the Copyright.txt
// file, which is part of this source code distribution.
//
// The AGS source code is provided under the Artistic License 2.0.
// A copy of this license can be found in the file License.txt and at
// https://opensource.org/license/artistic-2-0/
//
//=============================================================================
#include "data/dialogscriptconv.h"
#include <cctype>
#include <regex>
#include "util/memorystream.h"
#include "util/string_utils.h"
#include "util/textstreamreader.h"

using namespace AGS::Common;

namespace AGS
{
namespace DataUtil
{

const char *DialogScriptDefault = "\
#define DIALOG_NONE      0\n\
#define DIALOG_RUNNING   1\n\
#define DIALOG_STOP      2\n\
#define DIALOG_NEWROOM   100\n\
#define DIALOG_NEWTOPIC  12000\n\
\n\
_tryimport function dialog_request(int);\n\
int __dlgscript_tempval;\n\
\n\
function _run_dialog_request(int parmtr) {\n\
  game.stop_dialog_at_end = DIALOG_RUNNING;\n\
  dialog_request(parmtr);\n\
\n\
  if (game.stop_dialog_at_end == DIALOG_STOP) {\n\
    game.stop_dialog_at_end = DIALOG_NONE;\n\
    return -2;\n\
  }\n\
  if (game.stop_dialog_at_end >= DIALOG_NEWTOPIC) {\n\
    int tval = game.stop_dialog_at_end - DIALOG_NEWTOPIC;\n\
    game.stop_dialog_at_end = DIALOG_NONE;\n\
    return tval;\n\
  }\n\
  if (game.stop_dialog_at_end >= DIALOG_NEWROOM) {\n\
    int roomnum = game.stop_dialog_at_end - DIALOG_NEWROOM;\n\
    game.stop_dialog_at_end = DIALOG_NONE;\n\
    player.ChangeRoom(roomnum);\n\
    return -2;\n\
  }\n\
  game.stop_dialog_at_end = DIALOG_NONE;\n\
  return -1;\n\
}\n\
";


DialogScriptConverter::DialogScriptConverter(const String &dlg_script, const GameRef &game, const DialogRef &dialog)
    : _dlgScript(dlg_script), _game(game), _dialog(dialog)
{
    _scriptName.Format("Dialog %d", _dialog.ID);
    _sayFnName = _game.Settings.SayFunction;
    _narrateFnName = _game.Settings.NarrateFunction;
    // Fixups
    if (_sayFnName.IsNullOrSpace())
        _sayFnName = "Say";
    if (_narrateFnName.IsNullOrSpace())
        _narrateFnName = "Display";
}

String DialogScriptConverter::Convert()
{
    _errors.clear();
    _currentlyInsideCodeArea = false;
    _hadFirstEntryPoint = false;
    _entryPoints.clear();
    _lineNumber = 0;

    // TODO: TextStreamReader now deletes stream in dtor, which is a design mistake
    MemoryStream *mems = new MemoryStream((uint8_t*)_dlgScript.GetCStr(), _dlgScript.GetLength());
    TextStreamReader sr(mems);

    String ags_script =
        String::FromFormat("function _run_dialog%d(int entryPoint) { \n", _dialog.ID);

    String thisLine;
    for (thisLine = sr.ReadLine(); !thisLine.IsEmpty(); thisLine = sr.ReadLine())
    {
        _lineNumber++;
        String s = ConvertLine(thisLine);
        if (!s.IsNullOrSpace())
        {
            ags_script.Append(s);
            ags_script.Append("\n");
        }
    }
    if (_currentlyInsideCodeArea)
    {
        ags_script.Append("}\n");
    }
    ags_script.Append("return RUN_DIALOG_RETURN; }\n"); // end the function
    return ags_script;
}

void DialogScriptConverter::CompileError(const String &msg)
{
    _errors.push_back(CompileMessage(true, msg, _scriptName, _lineNumber));
}

void DialogScriptConverter::CompileWarning(const String &msg)
{
    _errors.push_back(CompileMessage(false, msg, _scriptName, _lineNumber));
}

bool DialogScriptConverter::IsRealScriptLine(const String &line)
{
    return ((line.GetAt(0) == ' ') || (line.GetAt(0) == '\t')) &&
        !line.IsNullOrSpace();
}

String DialogScriptConverter::ConvertLine(const String &line)
{
    if (IsRealScriptLine(line))
    {
        if (!_currentlyInsideCodeArea)
        {
            CompileError("Script commands can only be used in the area between a @ entry point and the closing return/stop statement");
            return "";
        }
        return ConvertRealScript(line);
    }
    return ConvertDialogScript(line);
}

// NOTE: original CJ's comment:
// We need this hideously complicated function because we don't
// want to replace it if "this" appears inside a string, hence
// we can't just use a simple regex
String DialogScriptConverter::ConvertRealScript(const String &src_line)
{
    String line = src_line;
    size_t at = -1;
    for (at = line.FindString("this", at + 1); at != -1;
        at = line.FindString("this", at + 1))
    {
        if (std::isalnum(line[at - 1]) ||
            (line[at - 1] == '_'))
        {
            continue;
        }
        if ((at < line.GetLength() - 4) &&
            ((std::isalnum(line[at + 4]) ||
            (line[at + 4] == '_'))))
        {
            continue;
        }
        bool insideString = false;
        for (size_t i = 1; i < at; i++)
        {
            if ((line[i] == '"') && (line[i - 1] != '\\'))
            {
                insideString = !insideString;
            }
        }
        if (!insideString)
        { // replace "this" with "dialog[X]"
            line.ReplaceMid(at, 4, String::FromFormat("dialog[%d]", _dialog.ID).GetCStr());
        }
    }
    return line;
}

String DialogScriptConverter::ConvertDialogScript(const String &src_line)
{
    String line = src_line;
    // look for comments and cut them off
    size_t at = line.FindString("//");
    if (at != -1)
    {
        line = line.Mid(0, at);
    }
    line.Trim();

    if (line.IsEmpty())
    {
        return "";
    }
    else if (line.StartsWith("@"))
    {
        return ProcessEntryPointTag(line);
    }
    else if (!_currentlyInsideCodeArea)
    {
        CompileWarning(
            String::FromFormat("The command '%s' will be ignored since the script for this option has already finished", line.GetCStr()));
        return "";
    }
    else if (line.FindChar(':') != -1)
    {
        return ProcessCharacterSpeech(line);
    }
    else
    {
        String script_line;
        // the old dialog script compiler allowed semicolons,
        // so just strip them out
        line.MakeLower();
        line.Replace(';', ' ');
        line.Trim();

        if (line == "return")
        {
            script_line = "return RUN_DIALOG_RETURN; }";
            _currentlyInsideCodeArea = false;
        }
        else if (line == "stop")
        {
            script_line = "return RUN_DIALOG_STOP_DIALOG; }";
            _currentlyInsideCodeArea = false;
        }
        else if (line.StartsWith("option-off-forever"))
        {
            script_line = ProcessOptionOnOff(line, "eOptionOffForever");
        }
        else if (line.StartsWith("option-off"))
        {
            script_line = ProcessOptionOnOff(line, "eOptionOff");
        }
        else if (line.StartsWith("option-on"))
        {
            script_line = ProcessOptionOnOff(line, "eOptionOn");
        }
        else if (line == "goto-previous")
        {
            script_line = "return RUN_DIALOG_GOTO_PREVIOUS; }";
            _currentlyInsideCodeArea = false;
        }
        else if (line.StartsWith("set-speech-view"))
        {
            script_line = ProcessCmdSpeechView(line);
        }
        else if (line.StartsWith("set-globalint"))
        {
            script_line = ProcessCmdSetGlobalInt(line);
        }
        else if (line.StartsWith("goto-dialog"))
        {
            script_line = ProcessCmdGotoDialog(line);
            _currentlyInsideCodeArea = false;
        }
        else if (line.StartsWith("run-script"))
        {
            script_line = ProcessCmdRunScript(line);
        }
        else if (line.StartsWith("play-sound"))
        {
            script_line = ProcessCmdArgInt(line, "play-sound", "PlaySound(%s);");
        }
        else if (line.StartsWith("add-inv"))
        {
            script_line = ProcessCmdArgInt(line, "add-inv", "player.AddInventory(inventory[%s]);");
        }
        else if (line.StartsWith("lose-inv"))
        {
            script_line = ProcessCmdArgInt(line, "lose-inv", "player.LoseInventory(inventory[%s]);");
        }
        else if (line.StartsWith("new-room"))
        {
            script_line = ProcessCmdArgInt(line, "new-room", "player.ChangeRoom(%s);");
            script_line.Append(" return RUN_DIALOG_STOP_DIALOG; }");
            _currentlyInsideCodeArea = false;
        }
        else if (line.StartsWith("give-score"))
        {
            script_line = ProcessCmdArgInt(line, "give-score", "GiveScore(%s);");
        }
        else
        {
            CompileError(
                String::FromFormat("Unknown command: %s. The command may require parameters which you have not supplied.", line.GetCStr()));
        }
        return script_line;
    }
}

String DialogScriptConverter::ProcessCmdArgInt(const String &line, const char *command, const char *replacement)
{
    //Match result = Regex.Match(dlgScriptLine, string.Format(@"^{0}\s*(\d+)$", command), RegexOptions.IgnoreCase);
    String pattern = String::FromFormat("^%s\\s*(\\d+)$", command);
    const std::regex regex(pattern.GetCStr(), std::regex_constants::icase);
    std::cmatch mr;
    if (!std::regex_match(line.GetCStr(), mr, regex))
    {
        CompileError(String::FromFormat("Invalid/missing parameter for %s", command));
        return "";
    }
    return String::FromFormat(replacement, mr[1].str().c_str());
}

String DialogScriptConverter::ProcessCmdRunScript(const String &line)
{
    //Match result = Regex.Match(dlgScriptCommand, @"^run-script\s*(\d+)$", RegexOptions.IgnoreCase);
    const std::regex regex("^run-script\\s*(\\d+)$", std::regex_constants::icase);
    std::cmatch mr;
    if (!std::regex_match(line.GetCStr(), mr, regex))
    {
        CompileError("run-script must supply dialog request ID");
        return "";
    }
    return String::FromFormat(
        "__dlgscript_tempval=_run_dialog_request(%s); if(__dlgscript_tempval!=-1) return __dlgscript_tempval;",
        mr[1].str().c_str());
}

String DialogScriptConverter::ProcessCmdGotoDialog(const String &line)
{
    //Match result = Regex.Match(dlgScriptCommand, @"^goto-dialog\s*(\w+)$", RegexOptions.IgnoreCase);
    const std::regex regex("^goto-dialog\\s*(\\w+)$", std::regex_constants::icase);
    std::cmatch mr;
    if (!std::regex_match(line.GetCStr(), mr, regex))
    {
        CompileError("goto-dialog must supply new dialog name");
        return "";
    }
    
    String dialog_name = mr[1].str().c_str();
    int dialogid;
    if (StrUtil::StringToInt(dialog_name, dialogid, 0) == StrUtil::kNoError)
    {
        return String::FromFormat("return %d;}", dialogid);
    }

    for (const auto &other_dialog : _game.Dialogs)
    {
        if (other_dialog.ScriptName.CompareNoCase(dialog_name) == 0)
        {
            return String::FromFormat("return %d;}", other_dialog.ID);
        }
    }

    CompileError(String::FromFormat("Dialog not found: %s", dialog_name.GetCStr()));
    return "";
}

String DialogScriptConverter::ProcessCmdSetGlobalInt(const String &line)
{
    //Match result = Regex.Match(dlgScriptCommand, @"^set-globalint\s*(\d+),?\s*(\d+)$", RegexOptions.IgnoreCase);
    const std::regex regex("^set-globalint\\s*(\\d+),?\\s*(\\d+)$", std::regex_constants::icase);
    std::cmatch mr;
    if (!std::regex_match(line.GetCStr(), mr, regex))
    {
        CompileError("set-globalint must supply global int number and new value");
        return "";
    }
    return String::FromFormat("SetGlobalInt(%s,%s);", mr[1].str().c_str(), mr[2].str().c_str());
}

String DialogScriptConverter::ProcessCmdSpeechView(const String &line)
{
    //Match result = Regex.Match(dlgScriptCommand, @"^set-speech-view\s*(\w+),?\s*(\d+)$", RegexOptions.IgnoreCase);
    const std::regex regex("^set-speech-view\\s*(\\w+),?\\s*(\\d+)$", std::regex_constants::icase);
    std::cmatch mr;
    if (!std::regex_match(line.GetCStr(), mr, regex))
    {
        CompileError("set-speech-view must supply character name and view number");
        return "";
    }
    String char_name = mr[1].str().c_str();
    // = result.Groups[2].Captures[0].Value;
    const auto *character = FindCharacterByScriptName(char_name);
    if (!character)
        return "";
    return String::FromFormat("%s.SpeechView = %s;", character->ScriptName.GetCStr(), mr[2].str().c_str());
}

String DialogScriptConverter::ProcessOptionOnOff(const String &line, const char *option_state)
{
    //Match result = Regex.Match(dlgScriptCommand, @"^option-(on|off(\-forever)?)\s*(\d+)$", RegexOptions.IgnoreCase);
    const std::regex regex("^option-(on|off(\\-forever)?)\\s*(\\d+)$", std::regex_constants::icase);
    std::cmatch mr;
    if (!std::regex_match(line.GetCStr(), mr, regex))
    {
        CompileError("option-on/off must supply option number");
        return "";
    }

    String option_str = mr[3].str().c_str();
    int option_num;
    if (StrUtil::StringToInt(option_str, option_num, 0) == StrUtil::kNoError)
    {
        if ((option_num < 1) || (option_num > _dialog.OptionCount))
        {
            CompileError(String::FromFormat("Dialog does not have an option number %d", option_num));
            return "";
        }
    }
    else
    {
        CompileError(String::FromFormat("Invalid option number: %s", option_str.GetCStr()));
        return "";
    }
    return String::FromFormat("dialog[%d].SetOptionState(%d,%s);", _dialog.ID, option_num, option_state);
}

String DialogScriptConverter::MakeCharacterSpeech(const String &name, const String &say_text)
{
    // Note that textToSay does not have to be a literal string, could be a function call for example
    if (name == "player")
    {
        return String::FromFormat("player.%s(%s);", _sayFnName.GetCStr(), say_text.GetCStr());
    }
    else if (name == "narrator")
    {
        return String::FromFormat("%s(%s);", _narrateFnName.GetCStr(), say_text.GetCStr());
    }
    else
    {
        const auto *character = FindCharacterByScriptName(name);
        if (!character)
            return "";
        return String::FromFormat("%s.%s(%s);", character->ScriptName.GetCStr(), _sayFnName.GetCStr(), say_text.GetCStr());
    }
}


String DialogScriptConverter::ProcessCharacterSpeech(const String &line)
{
    size_t colon_at = line.FindChar(':');
    String char_name = line.Mid(0, colon_at);
    char_name.Trim();
    char_name.MakeLower();
    String say_text = line.Mid(colon_at + 1);
    say_text.Trim();
    if (say_text.IsEmpty())
        say_text = "...";
    else
        ;// say_text.Replace("\"", "\\\""); // TODO: Replace strings
    say_text = String::FromFormat("\"%s\"", say_text.GetCStr());
    return MakeCharacterSpeech(char_name, say_text);
}

// TODO: move this somewhere else (game utility functions?)
// Finds character by a *case insensitive* script name
// (they don't have case sensitivity in dialog scripts)
const CharacterRef *DialogScriptConverter::FindCharacterByScriptName(const String &name)
{
    for (const auto &c : _game.Characters)
    {
        if (c.ScriptName.CompareNoCase(name) == 0 ||
            // also compare to a c-prefixed name
            (c.ScriptName.GetAt(0) == 'c' && c.ScriptName.CompareMidNoCase(name, 1) == 0))
        {
            return &c;
        }
    }

    CompileError(String::FromFormat("Unknown character: %s", name.GetCStr()));
    return nullptr;
}

String DialogScriptConverter::ProcessEntryPointTag(const String &line)
{
    int entryp_id;
    if (StrUtil::StringToInt(line.Mid(1), entryp_id, 0) == StrUtil::kNoError)
    {
    }
    else if ((line.GetLength() > 1) && (line.GetAt(1) == 'S'))
    {
        entryp_id = 0;
    }
    else
    {
        CompileError(String::FromFormat("Invalid entry point tag: %s", line.GetCStr()));
        return "";
    }
    if (std::find(_entryPoints.begin(), _entryPoints.end(), entryp_id) != _entryPoints.end())
    {
        CompileError(String::FromFormat("Entry point already exists in this script: %s", line.GetCStr()));
        return "";
    }
    if ((entryp_id < 0) || (entryp_id > _dialog.OptionCount))
    {
        CompileError(String::FromFormat("Dialog has no option number %d", entryp_id));
        return "";
    }
    _entryPoints.push_back(entryp_id);

    String script;
    if (_currentlyInsideCodeArea)
    {
        // the previous clause didn't have a return/stop/etc, so
        // make it flow into this one
        script = String::FromFormat("entryPoint = %d; }", entryp_id);
    }
    else if (_hadFirstEntryPoint)
    {
        script = "else ";
    }

    script.Append(String::FromFormat("if (entryPoint == %d) {", entryp_id));
    _hadFirstEntryPoint = true;
    _currentlyInsideCodeArea = true;
    return script;
}

} // namespace DataUtil
} // namespace AGS
