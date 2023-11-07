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
//
// DialogScriptConverter converts dialog script dialect into the regular
// AGS script which can be compiled by the AGS script compiler.
//
//=============================================================================
#ifndef __AGS_TOOL_DATA__DIALOGSCRIPTCONV_H
#define __AGS_TOOL_DATA__DIALOGSCRIPTCONV_H

#include "util/string.h"
#include "data/game_utils.h"
#include "data/script_utils.h"

namespace AGS
{
namespace DataUtil
{

using AGS::Common::String;

extern const char *DialogScriptDefault;

// DialogScriptConverter converts dialog script into the real AGS script
class DialogScriptConverter
{
public:
    DialogScriptConverter(const String &dlg_script, const GameRef &game, const DialogRef &dialog);

    String Convert();
    const CompileMessages &GetErrors() const { return _errors; }

private:
    void CompileError(const String &msg);
    void CompileWarning(const String &msg);
    static bool IsRealScriptLine(const String &line);
    String ConvertLine(const String &line);
    String ConvertRealScript(const String &line);
    String ConvertDialogScript(const String &line);
    String ProcessCmdArgInt(const String &line, const char *command, const char *replacement);
    String ProcessCmdRunScript(const String &line);
    String ProcessCmdGotoDialog(const String &line);
    String ProcessCmdSetGlobalInt(const String &line);
    String ProcessCmdSpeechView(const String &line);
    String ProcessOptionOnOff(const String &line, const char *option_state);
    String MakeCharacterSpeech(const String &name, const String &say_text);
    String ProcessCharacterSpeech(const String &line);
    String ProcessEntryPointTag(const String &line);
    const CharacterRef *FindCharacterByScriptName(const String &name);


    CompileMessages _errors;
    const GameRef &_game;
    const DialogRef &_dialog;
    String _scriptName;
    String _dlgScript;

    String _sayFnName;
    String _narrateFnName;
    bool _currentlyInsideCodeArea;
    bool _hadFirstEntryPoint;
    std::vector<int> _entryPoints;
    int _lineNumber;
};

} // namespace AGF
} // namespace AGS

#endif // __AGS_TOOL_DATA__DIALOGSCRIPTCONV_H
