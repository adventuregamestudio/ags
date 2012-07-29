using System;
using System.Collections.Generic;
using System.Text;
using AGS.Types.AutoComplete;
using AGS.Types;
using System.Windows.Forms;
using AGS.Types.Interfaces;

namespace AGS.Editor.TextProcessing
{
    public class FindAllUsages
    {
        ScintillaWrapper _scintilla;
        ScriptEditor _scriptEditor;
        AGSEditor _agsEditor;
        Script _script;
        List<ScriptTokenReference> _results;

        public FindAllUsages(ScintillaWrapper scintilla, ScriptEditor editor, 
            Script script, AGSEditor agsEditor)
        {
            this._scriptEditor = editor;
            this._scintilla = scintilla;
            this._script = script;
            this._agsEditor = agsEditor;
            this._results = new List<ScriptTokenReference>();
        }

        public void Find(string structName, string memberName)
        {
            //string token = string.IsNullOrEmpty(structName) ? memberName :
            //    string.Format("{0}.{1}", structName, memberName);
            string token = memberName;

            int? startSearchAtLineIndex;
            int? endSearchAtLineIndex;
            List<IScript> scriptsToSearch = GetScriptsToSearch(token,
                out startSearchAtLineIndex, out endSearchAtLineIndex);
            foreach (IScript script in scriptsToSearch)
            {
                if (script == null) continue;
                _scintilla = new ScintillaWrapper();                
                _scintilla.SetText(script.Text);            
                FindAllUsagesInScript(token, startSearchAtLineIndex, endSearchAtLineIndex, script);
            }

            ShowResults();
        }

        private void AddToResultsIfNeeded(IScript script, string line, 
            int characterIndex, int lineIndex, string token)
        {
            if (!_scintilla.InsideStringOrComment(false, characterIndex))
            {
                _results.Add(new ScriptTokenReference
                {
                    CurrentLine = line,
                    CharacterIndex = characterIndex,
                    Script = script,
                    LineIndex = lineIndex,
                    Token = token
                });
            }
        }

        private List<IScript> GetScriptsToSearch(string token, out int? startSearchAtLineIndex, out int? endSearchAtLineIndex)
        {
            List<IScript> scriptsToSearch = null;
            startSearchAtLineIndex = null;
            endSearchAtLineIndex = null;
            ScriptToken scriptToken = null;
            if (_scriptEditor != null && _scintilla != null)
            {
                scriptToken = _scriptEditor.FindTokenAsLocalVariable(token, true);
            }
            if (scriptToken != null)
            {
                ScriptFunction function = _scintilla.FindFunctionAtCurrentPosition();
                if (function != null)
                {
                    scriptsToSearch = new List<IScript> { _script };
                    startSearchAtLineIndex = _scintilla.FindLineNumberForCharacterIndex(function.StartsAtCharacterIndex);
                    endSearchAtLineIndex = _scintilla.FindLineNumberForCharacterIndex(function.EndsAtCharacterIndex);
                }
            }
            
            if (scriptsToSearch == null)
            {
                ScriptStruct scriptStruct = null;
                if (_scriptEditor != null)
                {
                    scriptStruct = _scriptEditor.FindGlobalVariableOrType(token);
                }
                if (scriptStruct == null)
                {
                    scriptsToSearch = _agsEditor.GetAllScripts();
                }
                else
                {
                    scriptsToSearch = new List<IScript> { _script };
                }
            }
            return scriptsToSearch;
        }
        
        private void ShowResults()
        {
            if (_results.Count == 0)
            {
                Factory.GUIController.ShowMessage("No usages were found!", MessageBoxIcon.Information);                
            }
            else if (_results.Count == 1)
            {
                Factory.GUIController.ZoomToFile(_results[0].Script.FileName, ZoomToFileZoomType.ZoomToCharacterPosition,
                    _results[0].CharacterIndex);
                Factory.GUIController.ShowMessage("This is the only usage!", MessageBoxIcon.Information);  
            }                            
            else
            {
                Factory.GUIController.ShowFindSymbolResults(_results);
            }
        }

        private void FindAllUsagesInScript(string token, int? startSearchAtLineIndex, int? endSearchAtLineIndex, IScript script)
        {            
            string[] lines = script.Text.Split('\n');
            int startLineCharacterIndex = 0;
            int startSearchAtLine = startSearchAtLineIndex.HasValue ?
                startSearchAtLineIndex.Value : 0;
            int endSearchAtLine = endSearchAtLineIndex.HasValue ?
                endSearchAtLineIndex.Value : lines.Length;
            for (int lineIndex = 0; lineIndex < endSearchAtLine; lineIndex++)
            {
                string line = lines[lineIndex];
                if (lineIndex >= startSearchAtLine && line.Contains(token))
                {
                    string[] splitLine = line.Split(new string[] { token }, StringSplitOptions.None);

                    if (AddTokenIfAloneInLine(splitLine, script,
                        line, startLineCharacterIndex, lineIndex, token)) continue;

                    if (AddTokenIfFirstInLine(splitLine, script,
                        line, startLineCharacterIndex, lineIndex, token)) continue;

                    if (AddTokenIfLastInLine(splitLine, script,
                        line, startLineCharacterIndex, lineIndex, token)) continue;

                    AddTokenIfMiddleInLine(splitLine, script,
                        line, startLineCharacterIndex, lineIndex, token);
                }
                startLineCharacterIndex += line.Length + 1;
            }
        }

        private bool IsCharAPartOfWord(char c)
        {
            return char.IsLetterOrDigit(c) || c == '_';
        }

        private bool AddTokenIfAloneInLine(string[] splitLine, IScript script, 
            string line, int startLineCharacterIndex, int lineIndex, string token)
        {
            if (splitLine == null || splitLine.Length == 0)
            {
                AddToResultsIfNeeded(script, line,
                    startLineCharacterIndex, lineIndex, token);
                return true;
            }
            return false;
        }

        private bool AddTokenIfFirstInLine(string[] splitLine,IScript script, 
            string line, int startLineCharacterIndex, int lineIndex, string token)
        {
            if (line.StartsWith(token) && !string.IsNullOrEmpty(splitLine[0])
                        && !IsCharAPartOfWord(splitLine[0][0]))
            {
                AddToResultsIfNeeded(script, line,
                            startLineCharacterIndex, lineIndex, token);
                return true;
            }
            return false;
        }

        private bool AddTokenIfLastInLine(string[] splitLine, IScript script, 
            string line, int startLineCharacterIndex, int lineIndex, string token)
        {
            string beforeLastToken = splitLine[splitLine.Length - 1];
            if (line.EndsWith(token) && !string.IsNullOrEmpty(beforeLastToken)
                && !IsCharAPartOfWord(beforeLastToken[beforeLastToken.Length - 1]))
            {
                int index = startLineCharacterIndex + line.Length - token.Length;
                AddToResultsIfNeeded(script, line,
                    index, lineIndex, token);
                return true;
            }
            return false;
        }

        private void AddTokenIfMiddleInLine(string[] splitLine, IScript script, 
            string line, int startLineCharacterIndex, int lineIndex, string token)
        {
            int length = 0;
            for (int i = 0; i < splitLine.Length - 1; i++)
            {
                string beforeToken = splitLine[i];
                string afterToken = splitLine[i + 1];
                length += beforeToken.Length;
                if (!string.IsNullOrEmpty(beforeToken) &&
                    !string.IsNullOrEmpty(afterToken) &&
                    !IsCharAPartOfWord(beforeToken[beforeToken.Length - 1]) &&
                    !IsCharAPartOfWord(afterToken[0]))
                {
                    int index = startLineCharacterIndex + length;
                    AddToResultsIfNeeded(script,
                            line, index, lineIndex, token);
                    break;
                }
                length += token.Length;
            }
        }        
    }
}
