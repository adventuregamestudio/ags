using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using AGS.Types;

namespace AGS.Editor
{
    internal class DialogScriptConverter
    {
        private int _currentLineNumber;
        private Dialog _currentDialog;
        private bool _currentlyInsideCodeArea = false;
        private bool _hadFirstEntryPoint = false;
        private Dictionary<int, object> _existingEntryPoints = new Dictionary<int, object>();
        private Game _game;
        private string _sayFnName;
        private string _narrateFnName;

        private static readonly string _DefaultDialogScriptsScript = null;

        static DialogScriptConverter()
        {
            _DefaultDialogScriptsScript = Resources.ResourceManager.GetResourceAsString("__DialogScripts.asc");
        }

        public string ConvertGameDialogScripts(Game game, CompileMessages errors, bool rebuildAll)
        {
            int stringBuilderCapacity = 1000 * game.RootDialogFolder.GetAllItemsCount() + _DefaultDialogScriptsScript.Length;
            StringBuilder sb = new StringBuilder(_DefaultDialogScriptsScript, stringBuilderCapacity);

            foreach (Dialog dialog in game.RootDialogFolder.AllItemsFlat)
            {
                sb.AppendLine(AGS.CScript.Compiler.Constants.NEW_SCRIPT_MARKER + "Dialog " + dialog.ID + "\"");

                if ((dialog.CachedConvertedScript == null) ||
                    (dialog.ScriptChangedSinceLastConverted) ||
                    (rebuildAll))
                {
                    int errorCountBefore = errors.Count;
                    this.ConvertDialogScriptToRealScript(dialog, game, errors);

                    if (errors.Count > errorCountBefore)
                    {
                        dialog.ScriptChangedSinceLastConverted = true;
                    }
                    else
                    {
                        dialog.ScriptChangedSinceLastConverted = false;
                    }
                }

                sb.Append(dialog.CachedConvertedScript);
            }

            return sb.ToString();
        }

        public void ConvertDialogScriptToRealScript(Dialog dialog, Game game, CompileMessages errors)
        {
            string thisLine;
            _currentlyInsideCodeArea = false;
            _hadFirstEntryPoint = false;
            _game = game;
            _sayFnName = game.Settings.DialogScriptSayFunction;
            if (string.IsNullOrWhiteSpace(_sayFnName))
                _sayFnName = "Say";
            _narrateFnName = game.Settings.DialogScriptNarrateFunction;
            if (string.IsNullOrWhiteSpace(_narrateFnName))
                _narrateFnName = "Display";
            _currentDialog = dialog;
            _existingEntryPoints.Clear();
            _currentLineNumber = 0;

            StringReader sr = new StringReader(dialog.Script);
            StringWriter sw = new StringWriter();
            sw.Write(string.Format("function _run_dialog{0}(int entryPoint) {1} ", dialog.ID, "{"));
            while ((thisLine = sr.ReadLine()) != null)
            {
                _currentLineNumber++;
                try
                {
                    ConvertDialogScriptLine(thisLine, sw, errors);
                }
                catch (CompileMessage ex)
                {
                    errors.Add(ex);
                }
            }
            if (_currentlyInsideCodeArea)
            {
                sw.WriteLine("}");
            }
            sw.WriteLine("return RUN_DIALOG_RETURN; }"); // end the function
            dialog.CachedConvertedScript = sw.ToString();
            sw.Close();
            sr.Close();
        }

        public static bool IsRealScriptLineInDialog(string theLine)
        {
            return ((theLine.StartsWith(" ") || theLine.StartsWith("\t")) &&
                   (theLine.Trim().Length > 0));
        }

        private void ConvertDialogScriptLine(string thisLine, StringWriter sw, CompileMessages errors)
        {
            if (IsRealScriptLineInDialog(thisLine))
            {
                if (!_currentlyInsideCodeArea)
                {
                    RaiseDialogScriptCompileError("Script commands can only be used in the area between a @ entry point and the closing return/stop statement");
                }
                thisLine = ConvertThisKeywordToDialogScriptName(thisLine);
                sw.WriteLine(thisLine);
            }
            else
            {
                ConvertDialogScriptCommandToScript(thisLine, sw, errors);
            }
        }

        /// <summary>
        /// We need this hideously complicated function because we don't
        /// want to replace it if "this" appears inside a string, hence
        /// we can't just use a simple regex
        /// </summary>
        private string ConvertThisKeywordToDialogScriptName(string thisLine)
        {
            int thisAtIndex = -1;
            while ((thisAtIndex = thisLine.IndexOf("this", thisAtIndex + 1)) > 0)
            {
                if (Char.IsLetterOrDigit(thisLine[thisAtIndex - 1]) ||
                    (thisLine[thisAtIndex - 1] == '_'))
                {
                    continue;
                }
                if ((thisAtIndex < thisLine.Length - 4) &&
                    ((Char.IsLetterOrDigit(thisLine[thisAtIndex + 4]) ||
                     (thisLine[thisAtIndex + 4] == '_'))))
                {
                    continue;
                }
                bool insideString = false;
                for (int i = 1; i < thisAtIndex; i++)
                {
                    if ((thisLine[i] == '"') && (thisLine[i - 1] != '\\'))
                    {
                        insideString = !insideString;
                    }
                }
                if (!insideString)
                {
                    thisLine = thisLine.Substring(0, thisAtIndex) + "dialog[" +
                               _currentDialog.ID + "]" + thisLine.Substring(thisAtIndex + 4);
                }
            }
            return thisLine;
        }

        private void ConvertDialogScriptCommandToScript(string dlgScriptCommand, StringWriter sw, CompileMessages errors)
        {
            string convertedScriptLine = string.Empty;
            if (dlgScriptCommand.Contains("//"))
            {
                dlgScriptCommand = dlgScriptCommand.Substring(0, dlgScriptCommand.IndexOf("//"));
            }
            dlgScriptCommand = dlgScriptCommand.Trim();

            if (dlgScriptCommand.Length == 0)
            {
                convertedScriptLine = string.Empty;
            }
            else if (dlgScriptCommand.StartsWith("@"))
            {
                convertedScriptLine = ProcessEntryPointTag(dlgScriptCommand);
            }
            else if (!_currentlyInsideCodeArea)
            {
                RaiseDialogScriptCompileWarning(string.Format("The command '{0}' will be ignored since the script for this option has already finished", dlgScriptCommand), errors);
                convertedScriptLine = string.Empty;
            }
            else if (dlgScriptCommand.IndexOf(':') > 0)
            {
                convertedScriptLine = ProcessCharacterSpeechLine(dlgScriptCommand);
            }
            else
            {
                // the old dialog script compiler allowed semicolons,
                // so just strip them out
                dlgScriptCommand = dlgScriptCommand.ToLower().Replace(";", "");
                
                if (dlgScriptCommand == "return")
                {
                    convertedScriptLine = "return RUN_DIALOG_RETURN; }";
                    _currentlyInsideCodeArea = false;
                }
                else if (dlgScriptCommand == "stop")
                {
                    convertedScriptLine = "return RUN_DIALOG_STOP_DIALOG; }";
                    _currentlyInsideCodeArea = false;
                }
                else if (dlgScriptCommand.StartsWith("option-off-forever"))
                {
                    convertedScriptLine = ProcessOptionOnOrOff(dlgScriptCommand, "eOptionOffForever");
                }
                else if (dlgScriptCommand.StartsWith("option-off"))
                {
                    convertedScriptLine = ProcessOptionOnOrOff(dlgScriptCommand, "eOptionOff");
                }
                else if (dlgScriptCommand.StartsWith("option-on"))
                {
                    convertedScriptLine = ProcessOptionOnOrOff(dlgScriptCommand, "eOptionOn");
                }
                else if (dlgScriptCommand == "goto-previous")
                {
                    convertedScriptLine = "return RUN_DIALOG_GOTO_PREVIOUS; }";
                    _currentlyInsideCodeArea = false;
                }
                else if (dlgScriptCommand.StartsWith("set-speech-view"))
                {
                    convertedScriptLine = ProcessSetSpeechViewCommand(dlgScriptCommand);
                }
                else if (dlgScriptCommand.StartsWith("set-globalint"))
                {
                    convertedScriptLine = ProcessSetGlobalIntCommand(dlgScriptCommand);
                }
                else if (dlgScriptCommand.StartsWith("goto-dialog"))
                {
                    convertedScriptLine = ProcessGotoDialogCommand(dlgScriptCommand);
                    _currentlyInsideCodeArea = false;
                }
                else if (dlgScriptCommand.StartsWith("run-script"))
                {
                    convertedScriptLine = ProcessRunScriptCommand(dlgScriptCommand);
                }
                else if (dlgScriptCommand.StartsWith("play-sound"))
                {
                    convertedScriptLine = ProcessSingleIntParameterCommand(dlgScriptCommand, "play-sound", "PlaySound({0});");
                }
                else if (dlgScriptCommand.StartsWith("add-inv"))
                {
                    convertedScriptLine = ProcessSingleIntParameterCommand(dlgScriptCommand, "add-inv", "player.AddInventory(inventory[{0}]);");
                }
                else if (dlgScriptCommand.StartsWith("lose-inv"))
                {
                    convertedScriptLine = ProcessSingleIntParameterCommand(dlgScriptCommand, "lose-inv", "player.LoseInventory(inventory[{0}]);");
                }
                else if (dlgScriptCommand.StartsWith("new-room"))
                {
                    convertedScriptLine = ProcessSingleIntParameterCommand(dlgScriptCommand, "new-room", "player.ChangeRoom({0}); return RUN_DIALOG_STOP_DIALOG;") + "}";
                    _currentlyInsideCodeArea = false;
                }
                else if (dlgScriptCommand.StartsWith("give-score"))
                {
                    convertedScriptLine = ProcessSingleIntParameterCommand(dlgScriptCommand, "give-score", "GiveScore({0});");
                }
                else
                {
                    RaiseDialogScriptCompileError("Unknown command: " + dlgScriptCommand + ". The command may require parameters which you have not supplied.");
                }
            }
            sw.WriteLine(convertedScriptLine);
        }

        private string ProcessSingleIntParameterCommand(string dlgScriptLine, string command, string scriptReplacement)
        {
            Match result = Regex.Match(dlgScriptLine, string.Format(@"^{0}\s*(\d+)$", command), RegexOptions.IgnoreCase);
            if (!result.Success)
            {
                RaiseDialogScriptCompileError("Invalid/missing parameter for " + command);
            }
            string parameterValue = result.Groups[1].Captures[0].Value;
            return string.Format(scriptReplacement, parameterValue);
        }

        private string ProcessRunScriptCommand(string dlgScriptCommand)
        {
            Match result = Regex.Match(dlgScriptCommand, @"^run-script\s*(\d+)$", RegexOptions.IgnoreCase);
            if (!result.Success)
            {
                RaiseDialogScriptCompileError("run-script must supply dialog request ID");
            }
            string dialogRequestID = result.Groups[1].Captures[0].Value;
            return string.Format("__dlgscript_tempval=_run_dialog_request({0}); if(__dlgscript_tempval!=-1) return __dlgscript_tempval;", dialogRequestID);
        }

        private string ProcessGotoDialogCommand(string dlgScriptCommand)
        {
            Match result = Regex.Match(dlgScriptCommand, @"^goto-dialog\s*(\w+)$", RegexOptions.IgnoreCase);
            if (!result.Success)
            {
                RaiseDialogScriptCompileError("goto-dialog must supply new dialog name");
            }
            string newDialogName = result.Groups[1].Captures[0].Value;
            int dialogID;
            if (int.TryParse(newDialogName, out dialogID))
            {
                return string.Format("return {0};", dialogID) + "}";
            }

            foreach (Dialog otherDialog in _game.RootDialogFolder.AllItemsFlat)
            {
                if (string.Compare(otherDialog.Name, newDialogName, true) == 0)
                {
                    return string.Format("return {0};", otherDialog.ID) + "}";
                }
            }

            RaiseDialogScriptCompileError("Dialog not found: " + newDialogName);
            return null;
        }

        private string ProcessSetGlobalIntCommand(string dlgScriptCommand)
        {
            Match result = Regex.Match(dlgScriptCommand, @"^set-globalint\s*(\d+),?\s*(\d+)$", RegexOptions.IgnoreCase);
            if (!result.Success)
            {
                RaiseDialogScriptCompileError("set-globalint must supply global int number and new value");
            }
            string globalIntNumber = result.Groups[1].Captures[0].Value;
            string newValue = result.Groups[2].Captures[0].Value;
            return string.Format("SetGlobalInt({0},{1});", globalIntNumber, newValue);
        }

        private string ProcessSetSpeechViewCommand(string dlgScriptCommand)
        {
            Match result = Regex.Match(dlgScriptCommand, @"^set-speech-view\s*(\w+),?\s*(\d+)$", RegexOptions.IgnoreCase);
            if (!result.Success)
            {
                RaiseDialogScriptCompileError("set-speech-view must supply character name and view number");
            }
            string charName = result.Groups[1].Captures[0].Value;
            string viewNumber = result.Groups[2].Captures[0].Value;
            Character character = FindCharacterByScriptName(charName.ToLower());
            return string.Format("{0}.SpeechView = {1};", character.ScriptName, viewNumber);
        }

        private string ProcessOptionOnOrOff(string dlgScriptCommand, string optionStateEnum)
        {
            Match result = Regex.Match(dlgScriptCommand, @"^option-(on|off(\-forever)?)\s*(\d+)$", RegexOptions.IgnoreCase);
            if (!result.Success)
            {
                RaiseDialogScriptCompileError("option-on/off must supply option number");
            }
            string optionNumber = result.Groups[3].Captures[0].Value;
            int optionNumberAsInt;
            if (int.TryParse(optionNumber, out optionNumberAsInt))
            {
                if ((optionNumberAsInt < 1) || (optionNumberAsInt > _currentDialog.Options.Count))
                {
                    RaiseDialogScriptCompileError("Dialog does not have an option number " + optionNumberAsInt.ToString());
                }
            }
            else
            {
                RaiseDialogScriptCompileError("Invalid option number: " + optionNumber);
            }
            return string.Format("dialog[{0}].SetOptionState({1}, {2});", _currentDialog.ID, optionNumber, optionStateEnum);
        }

        private string MakeCharacterSpeechLine(string charName, string textToSay)
        {
            // Note that textToSay does not have to be a literal string, could be a function call for example
            string scriptToReturn = string.Empty;
            if (charName == "player")
            {
                scriptToReturn = string.Format("player.{0}({1});", _sayFnName, textToSay);
            }
            else if (charName == "narrator")
            {
                scriptToReturn = string.Format("{0}({1});", _narrateFnName, textToSay);
            }
            else
            {
                Character character = FindCharacterByScriptName(charName);
                scriptToReturn = string.Format("{0}.{1}({2});", character.ScriptName, _sayFnName, textToSay);
            }
            return scriptToReturn;
        }

        private string ProcessCharacterSpeechLine(string dlgScriptCommand)
        {
            int colonAtIndex = dlgScriptCommand.IndexOf(':');
            string charName = dlgScriptCommand.Substring(0, colonAtIndex).Trim().ToLower();
            string textToSay = dlgScriptCommand.Substring(colonAtIndex + 1).Trim();
            if (textToSay == string.Empty)
                textToSay = "...";
            else
                textToSay = textToSay.Replace("\"", "\\\"");
            textToSay = string.Format("\"{0}\"", textToSay);
            return MakeCharacterSpeechLine(charName, textToSay);
        }

        private Character FindCharacterByScriptName(string scriptName)
        {
            string scriptNameWithCPrefix = "c" + scriptName;

            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                string thisCharName = character.ScriptName.ToLower();
                if ((thisCharName == scriptName) ||
                    (thisCharName == scriptNameWithCPrefix))
                {
                    return character;
                }
            }

            RaiseDialogScriptCompileError("Unknown character: " + scriptName);
            return null;
        }

        private string ProcessEntryPointTag(string dlgScriptCommand)
        {
            int entryPointID;
            if (int.TryParse(dlgScriptCommand.Substring(1), out entryPointID))
            {
            }
            else if ((dlgScriptCommand.Length > 1) && (dlgScriptCommand[1] == 'S'))
            {
                entryPointID = 0;
            }
            else
            {
                RaiseDialogScriptCompileError("Invalid entry point tag: " + dlgScriptCommand);
            }
            if (_existingEntryPoints.ContainsKey(entryPointID))
            {
                RaiseDialogScriptCompileError("Entry point already exists in this script: " + dlgScriptCommand);
            }
            if ((entryPointID < 0) || (entryPointID > _currentDialog.Options.Count))
            {
                RaiseDialogScriptCompileError("Dialog has no option number " + entryPointID.ToString());
            }
            _existingEntryPoints.Add(entryPointID, null);

            string scriptToWrite = string.Empty;
            if (_currentlyInsideCodeArea)
            {
                // the previous clause didn't have a return/stop/etc, so
                // make it flow into this one
                scriptToWrite += string.Format("entryPoint = {0}; {1}", entryPointID, "}");
            }
            else if (_hadFirstEntryPoint)
            {
                scriptToWrite += "else ";
            }

            scriptToWrite += string.Format("if (entryPoint == {0}) {1}", entryPointID, "{");
            _hadFirstEntryPoint = true;
            _currentlyInsideCodeArea = true;

            /* --- disabled until Dialog.DisplayOptions(eSayAlways/eSayNever) question is resolved ---
             *
            // Add player.Say if the Say checkbox is set for this option
            if (entryPointID > 0)
            {
                int optionID = entryPointID - 1;
                string dlgName = _currentDialog.Name;
                if (_currentDialog.Options[optionID].Say)
                    // Use GetOptionText here in case future script API supports changing option text at runtime
                    scriptToWrite += MakeCharacterSpeechLine("player", string.Format("{0}.GetOptionText({1})", dlgName, optionID + 1));
            }
            */

            return scriptToWrite;
        }

        private void RaiseDialogScriptCompileError(string errorMsg)
        {
            throw new CompileError(errorMsg, "Dialog " + _currentDialog.ID, _currentLineNumber);
        }

        private void RaiseDialogScriptCompileWarning(string errorMsg, CompileMessages errors)
        {
            errors.Add(new CompileWarning(errorMsg, "Dialog " + _currentDialog.ID, _currentLineNumber));
        }
    }
}
