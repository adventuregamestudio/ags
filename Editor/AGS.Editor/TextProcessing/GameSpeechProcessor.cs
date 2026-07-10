using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.Remoting.Contexts;
using System.Text;

namespace AGS.Editor
{
    public abstract class GameSpeechProcessor : IGameTextProcessor
    {
        protected const string NARRATOR_NAME = "narrator";
        protected const string PLAYER_NAME = "player";

        protected Game _game;
        protected CompileMessages _errors;
        private bool _makesChanges;
        private bool _processHotspotAndObjectDescriptions;
        private bool _lookupForFunctionCalls;
        private bool _lookupForOuterFunctionCalls;
        // Current parsing state
        protected string _scriptName;
        protected ScriptParsing.ParserState _parserState;

        protected abstract string CreateSpeechLine(GameTextLine textLine, GameTextType textType);
        protected abstract bool ParseFunctionCall(string scriptCodeExtract, out int characterID);

        public GameSpeechProcessor(Game game, CompileMessages errors, bool makesChanges,
            bool processHotspotAndObjectDescriptions, bool lookupForFunctionCalls, bool lookupForOuterFunctionCalls)
        {
            _game = game;
            _errors = errors;
            _makesChanges = makesChanges;
            _processHotspotAndObjectDescriptions = processHotspotAndObjectDescriptions;
            _lookupForFunctionCalls = lookupForFunctionCalls;
            _lookupForOuterFunctionCalls = lookupForOuterFunctionCalls;
        }

        public bool MakesChanges
        {
            get { return _makesChanges; }
        }

        protected bool LookupForFunctionCalls
        {
            get { return _lookupForFunctionCalls; }
            set { _lookupForFunctionCalls = value; }
        }

        protected bool LookupForOuterFunctionCalls
        {
            get { return _lookupForOuterFunctionCalls; }
            set { _lookupForOuterFunctionCalls = value; }
        }

        public string ProcessText(string text, GameTextType textType)
        {
            return ProcessText(new GameTextLine(text), textType);
        }

        public string ProcessText(string text, string sourceRef, GameTextType textType)
        {
            return ProcessText(new GameTextLine(text, sourceRef), textType);
        }
        public string ProcessText(GameTextLine textLine, GameTextType textType)
        {
            if (string.IsNullOrWhiteSpace(textLine.Text))
            {
                return string.Empty;
            }

            switch (textType) 
            {
                case GameTextType.DialogOption:
                case GameTextType.Message:
                    return CreateSpeechLine(textLine, textType);
                case GameTextType.DialogScript:
                    return ProcessDialogScript(textLine.Text, textLine.ScriptFileName, textLine.SourceRef);
                case GameTextType.Script:
                    return ProcessScript(textLine.Text, textLine.ScriptFileName, textLine.SourceRef);
                case GameTextType.ItemDescription:
                    if (_processHotspotAndObjectDescriptions)
                    {
                        return CreateSpeechLine(textLine, textType);
                    }
                    break;
                default:
                    return CreateSpeechLine(textLine, textType);
            }
            return textLine.Text;
        }

        private bool DoesStringTerminateHere(ScriptParsing.ParserState state, char stringTerminator)
        {
            if (state.Char == stringTerminator)
            {
                // We must make sure that this quotemark is not escaped, so count
                // number of backslashes before it. Should be either 0, or even number
                // (in the latter case they escape themselves and don't affect quotemark).
                int backslashes = 0;
                for (int index = state.CharIndex - 1; index >= 0 && state.Script[index] == '\\'; --index)
                    backslashes++;
                return (backslashes == 0) || (backslashes % 2 == 0);
            }
            return false;
        }

        private string ProcessScript(string script, string scriptName, string contextName, int originalLine = -1)
        {
            // If originalLine is set, then this "script" is an excerpt from another bigger text
            ScriptParsing.ParserState state = new ScriptParsing.ParserState(script, originalLine);
            ScriptParsing.FillLines(state);
            state.Reset();

            _scriptName = scriptName;
            _parserState = state;

            while (state.CharIndex < script.Length)
            {
				ScriptParsing.SkipComments(state);

                if (!state.AtEnd && 
                    ((state.Char == '"') || (state.Char == '\'')))
                {
                    char stringTerminator = state.Char;
                    int stringStartIndex = state.CharIndex;
                    state.Forward();
                    while (!state.AtEnd)
                    {
                        if (DoesStringTerminateHere(state, stringTerminator))
                        {
                            break;
                        }
                        state.Forward();
                    }
                    if (state.AtEnd)
                    {
                        _errors.Add(new CompileError("Unterminated string in script: " + script.Substring(stringStartIndex), scriptName, state.Line));
                        return script;
                    }

                    if (stringTerminator == '"')
                    {
                        int stringEndIndex = state.CharIndex;
                        string previousFuncCall = _lookupForFunctionCalls ?
                            ScriptParsing.GetCurrentFunctionCall(state, stringStartIndex, _lookupForOuterFunctionCalls)
                            : string.Empty;
                        int charID;
                        if (ParseFunctionCall(previousFuncCall, out charID))
                        {
                            string mainString = script.Substring(stringStartIndex + 1, (stringEndIndex - stringStartIndex) - 1);
                            string modifiedString = CreateSpeechLine(GameTextLine.MakeSpeechLine(charID, mainString, contextName, state.Line), GameTextType.Script);
                            if (_makesChanges)
                            {
                                string scriptBeforeString = script.Substring(0, stringStartIndex + 1);
                                string scriptAfterString = script.Substring(stringEndIndex);
                                script = scriptBeforeString + modifiedString + scriptAfterString;
                                state.SetAt(stringStartIndex + modifiedString.Length + 1);
                            }
                            else
                            {
                                state.SetAt(stringStartIndex + mainString.Length + 1);
                            }
                        }
                    }
                }
                state.Forward();
            }
            return script;
        }

        private string ProcessDialogScript(string script, string scriptFileName, string contextName)
        {
            StringBuilder sb = new StringBuilder(script.Length);
            StreamReader sr = new StreamReader(new MemoryStream(_game.TextEncoding.GetBytes(script), false), _game.TextEncoding);
            string thisLine;
            string originalLine;
            int line = 1;
            while ((thisLine = sr.ReadLine()) != null)
            {
                originalLine = thisLine;
				thisLine = thisLine.Trim();

                if (DialogScriptConverter.IsRealScriptLineInDialog(originalLine))
                {
                    originalLine = ProcessScript(originalLine, scriptFileName, contextName, line);
                }
                else if (thisLine.IndexOf("//") >= 0)
                {
                    thisLine = thisLine.Substring(0, thisLine.IndexOf("//"));
                }
                else if (thisLine.IndexOf(":") > 0)
                {
                    int characterNameLength = 0;
                    while (thisLine[characterNameLength].IsScriptWordChar())
                    {
                        characterNameLength++;
                    }
                    string characterName = thisLine.Substring(0, characterNameLength);
                    int charID = FindCharacterIDForCharacter(characterName);
                    if (charID >= 0)
                    {
                        string lineText = thisLine.Substring(thisLine.IndexOf(":") + 1).Trim();
                        originalLine = string.Format("{0}: {1}", characterName,
                            CreateSpeechLine(GameTextLine.MakeSpeechLine(charID, lineText, contextName, line), GameTextType.DialogScript));
                    }
                    else
                    {
                        _errors.Add(new CompileError("Unknown character: " + characterName, scriptFileName, line));
                    }
                }

                sb.AppendLine(originalLine);
                line++;
            }
            sr.Close();
            return sb.ToString();
        }

        private Character FindCharacterWithName(string characterName)
        {
            foreach (Character character in _game.RootCharacterFolder.AllItemsFlat)
            {
                if (character.ScriptName.ToLowerInvariant() == characterName)
                {
                    return character;
                }
            }
            return null;
        }

        protected int FindCharacterIDForCharacter(string characterName)
        {
            characterName = characterName.ToLowerInvariant();

            if (characterName == NARRATOR_NAME)
            {
                return Character.NARRATOR_CHARACTER_ID;
            }
            if (characterName == PLAYER_NAME)
            {
                return _game.PlayerCharacter.ID;
            }

            Character foundChar = FindCharacterWithName(characterName);
            if (foundChar == null)
            {
                foundChar = FindCharacterWithName("c" + characterName);
            }
            if (foundChar != null)
            {
                return foundChar.ID;
            }

            return -1;
        }

    }
}
