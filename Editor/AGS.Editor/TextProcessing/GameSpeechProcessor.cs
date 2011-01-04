using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public abstract class GameSpeechProcessor : IGameTextProcessor
    {
        private const int LOOKBACK_DISTANCE_FOR_FUNCTION_CALL = 35;

        protected const string NARRATOR_NAME = "narrator";
        protected const string PLAYER_NAME = "player";
        private const char NEWLINE_CHAR_1 = '\n';
        private const char NEWLINE_CHAR_2 = '\r';

        protected Game _game;
        protected CompileMessages _errors;
        private bool _makesChanges;
        private bool _processHotspotAndObjectDescriptions;

        protected abstract string CreateSpeechLine(int speakingCharacter, string text);
        protected abstract int ParseFunctionCallAndFindCharacterID(string scriptCodeExtract);

        public GameSpeechProcessor(Game game, CompileMessages errors, bool makesChanges, bool processHotspotAndObjectDescriptions)
        {
            _game = game;
            _errors = errors;
            _makesChanges = makesChanges;
            _processHotspotAndObjectDescriptions = processHotspotAndObjectDescriptions;
        }

        public bool MakesChanges
        {
            get { return _makesChanges; }
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
                    return CreateSpeechLine(characterID, text);
                case GameTextType.DialogScript:
                    return ProcessDialogScript(text);
                case GameTextType.Script:
                    return ProcessScript(text);
                case GameTextType.ItemDescription:
                    if (_processHotspotAndObjectDescriptions)
                    {
                        return CreateSpeechLine(characterID, text);
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

        private string GetPreviousCharacters(string script, int startingFromIndex)
        {
            int previousCodeStart = startingFromIndex;
            for (int i = 0; (i < LOOKBACK_DISTANCE_FOR_FUNCTION_CALL) && (previousCodeStart >= 0); i++)
            {
                if ((script[previousCodeStart] == NEWLINE_CHAR_1) ||
                    (script[previousCodeStart] == NEWLINE_CHAR_2))
                {
                    previousCodeStart++;
                    break;
                }
                previousCodeStart--;
            }
            if (previousCodeStart < 0)
            {
                previousCodeStart = 0;
            }
            return script.Substring(previousCodeStart, (startingFromIndex - previousCodeStart) + 1);
        }

		/// <summary>
		/// Skip comments in the script that might have speech marks
		/// in them, which could confuse the parser.
		/// </summary>
		private int SkipComments(string script, int index)
		{
			if ((index < script.Length - 1) &&
			    (script[index] == '/') && (script[index + 1] == '/'))
			{
				while ((index < script.Length) &&
					(script[index] != 10) &&
					(script[index] != 13))
				{
					index++;
				}
			}
			if ((index < script.Length - 1) &&
				(script[index] == '/') && (script[index + 1] == '*'))
			{
				index += 2;
				while (index < script.Length - 1)
				{
					if ((script[index] == '*') && (script[index + 1] == '/'))
					{
						break;
					}
					index++;
				}
			}
			return index;
		}

        private string ProcessScript(string script)
        {
            int index = 0;
            while (index < script.Length)
            {
				index = SkipComments(script, index);

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
                        string previousFuncCall = GetPreviousCharacters(script, stringStartIndex - 1);
                        int charID = ParseFunctionCallAndFindCharacterID(previousFuncCall);
                        if (charID >= 0)
                        {
                            string scriptBeforeString = script.Substring(0, stringStartIndex + 1);
                            string scriptAfterString = script.Substring(stringEndIndex);
                            string mainString = script.Substring(stringStartIndex + 1, (stringEndIndex - stringStartIndex) - 1);
                            string modifiedString = CreateSpeechLine(charID, mainString);
                            script = scriptBeforeString + modifiedString + scriptAfterString;
                            index = stringStartIndex + modifiedString.Length + 1;
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
            StreamReader sr = new StreamReader(new MemoryStream(Encoding.Default.GetBytes(script), false), Encoding.Default);
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
                    while (Char.IsLetterOrDigit(thisLine[characterNameLength]))
                    {
                        characterNameLength++;
                    }
                    string characterName = thisLine.Substring(0, characterNameLength);
                    int charID = FindCharacterIDForCharacter(characterName);
                    if (charID >= 0)
                    {
                        string lineText = thisLine.Substring(thisLine.IndexOf(":") + 1).Trim();
                        originalLine = string.Format("{0}: {1}", characterName, CreateSpeechLine(charID, lineText));
                    }
                }

                sb.AppendLine(originalLine);
            }
            sr.Close();
            return sb.ToString();
        }

        private Character FindCharacterWithName(string characterName)
        {
            foreach (Character character in _game.Characters)
            {
                if (character.ScriptName.ToLower() == characterName)
                {
                    return character;
                }
            }
            return null;
        }

        protected int FindCharacterIDForCharacter(string characterName)
        {
            characterName = characterName.ToLower();

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
