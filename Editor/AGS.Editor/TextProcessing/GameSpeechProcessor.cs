using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
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

        protected abstract string CreateSpeechLine(int speakingCharacter, string text, GameTextType textType);
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
            return ProcessText(text, textType, -1);
        }

        public string ProcessText(string text, GameTextType textType, int characterID)
        {
            if (text == null)
            {
                return string.Empty;
            }

            if (text.Trim() == string.Empty)
            {
                return text;
            }

            switch (textType) 
            {
                case GameTextType.DialogOption:
                case GameTextType.Message:
                    return CreateSpeechLine(characterID, text, textType);
                case GameTextType.DialogScript:
                    return ProcessDialogScript(text);
                case GameTextType.Script:
                    return ProcessScript(text);
                case GameTextType.ItemDescription:
                    if (_processHotspotAndObjectDescriptions)
                    {
                        return CreateSpeechLine(characterID, text, textType);
                    }
                    break;
            }
            return text;
        }

        private bool DoesStringTerminateHere(string script, int index, char stringTerminator)
        {
            if ((script[index] == stringTerminator) && (script[index - 1] != '\\'))
            {
                // speech mark (not escaped)
                return true;
            }
            if ((script[index] == stringTerminator) && (script[index - 1] == '\\') && (script[index - 2] == '\\'))
            {
                // speech mark, with escaped backslash before it
                return true;
            }
            return false;
        }

        private string ProcessScript(string script)
        {
            ScriptParsing.ParserState state = new ScriptParsing.ParserState(script);
            int index = 0;
            while (index < script.Length)
            {
				index = ScriptParsing.SkipComments(state, index);

                if ((index < script.Length) && 
                    ((script[index] == '"') || (script[index] == '\'')))
                {
                    char stringTerminator = script[index];
                    int stringStartIndex = index;
                    index++;
                    while (index < script.Length)
                    {
                        if (DoesStringTerminateHere(script, index, stringTerminator))
                        {
                            break;
                        }
                        index++;
                    }
                    if (index >= script.Length)
                    {
                        _errors.Add(new CompileError("Unterminated string in script: " + script.Substring(stringStartIndex)));
                        return script;
                    }

                    if (stringTerminator == '"')
                    {
                        int stringEndIndex = index;
                        string previousFuncCall = _lookupForFunctionCalls ?
                            ScriptParsing.GetCurrentFunctionCall(state, stringStartIndex, _lookupForOuterFunctionCalls)
                            : string.Empty;
                        int charID;
                        if (ParseFunctionCall(previousFuncCall, out charID))
                        {
                            string mainString = script.Substring(stringStartIndex + 1, (stringEndIndex - stringStartIndex) - 1);
                            string modifiedString = CreateSpeechLine(charID, mainString, GameTextType.Script);
                            if (_makesChanges)
                            {
                                string scriptBeforeString = script.Substring(0, stringStartIndex + 1);
                                string scriptAfterString = script.Substring(stringEndIndex);
                                script = scriptBeforeString + modifiedString + scriptAfterString;
                                index = stringStartIndex + modifiedString.Length + 1;
                            }
                            else
                            {
                                index = stringStartIndex + mainString.Length + 1;
                            }
                        }
                    }
                }
                index++;
            }
            return script;
        }

        private string ProcessDialogScript(string script)
        {
            StringBuilder sb = new StringBuilder(script.Length);
            StreamReader sr = new StreamReader(new MemoryStream(_game.TextEncoding.GetBytes(script), false), _game.TextEncoding);
            string thisLine;
            string originalLine;
            while ((thisLine = sr.ReadLine()) != null)
            {
                originalLine = thisLine;
				thisLine = thisLine.Trim();

                if (DialogScriptConverter.IsRealScriptLineInDialog(originalLine))
                {
                    originalLine = ProcessScript(originalLine);
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
                        originalLine = string.Format("{0}: {1}", characterName, CreateSpeechLine(charID, lineText, GameTextType.DialogScript));
                    }
                }

                sb.AppendLine(originalLine);
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

            _errors.Add(new CompileError("Unknown character name: " + characterName));
            return -1;
        }

    }
}
