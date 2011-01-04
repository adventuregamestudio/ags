using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
	public abstract class SpeechOnlyProcessor : GameSpeechProcessor
	{
		private Dictionary<string, FunctionCallType> _speechableFunctionCalls;

		public SpeechOnlyProcessor(Game game, CompileMessages errors, bool makesChanges, bool processHotspotAndObjectDescriptions,
			Dictionary<string, FunctionCallType> speechableFunctionCalls)
			: base(game, errors, makesChanges, processHotspotAndObjectDescriptions)
		{
			_speechableFunctionCalls = speechableFunctionCalls;
		}

		protected override int ParseFunctionCallAndFindCharacterID(string scriptCodeExtract)
		{
			foreach (string nameToSearchFor in _speechableFunctionCalls.Keys)
			{
				int index = scriptCodeExtract.IndexOf(nameToSearchFor);
				if (index >= 0)
				{
					if (_speechableFunctionCalls[nameToSearchFor] == FunctionCallType.GlobalNarrator)
					{
						return Character.NARRATOR_CHARACTER_ID;
					}
					int startParseForNameAt = 0;

					if (_speechableFunctionCalls[nameToSearchFor] == FunctionCallType.GlobalSpeech)
					{
						startParseForNameAt = FindIndexOfCharacterNameInGlobalFunctionCall(scriptCodeExtract);
					}
					else
					{
						startParseForNameAt = FindIndexOfCharacterNameInObjectFunctionCall(scriptCodeExtract);
					}

					int endIndex = startParseForNameAt;
					while ((endIndex < scriptCodeExtract.Length) && (Char.IsLetterOrDigit(scriptCodeExtract[endIndex])))
					{
						endIndex++;
					}

					if (endIndex == startParseForNameAt)
					{
						throw new AGSEditorException("Error parsing script at " + scriptCodeExtract);
					}

					string characterName = scriptCodeExtract.Substring(startParseForNameAt, (endIndex - startParseForNameAt));
					if (Char.IsDigit(characterName[0]))
					{
						return ParseNumericalCharacterID(characterName);
					}
					return FindCharacterIDForCharacter(characterName);
				}
			}
			return -1;
		}

		private int FindIndexOfCharacterNameInObjectFunctionCall(string scriptCodeExtract)
		{
			int checkIndex = scriptCodeExtract.Length - 1;
			while ((checkIndex > 0) && (scriptCodeExtract[checkIndex] != '('))
			{
				checkIndex--;
			}
			while ((checkIndex > 0) && (scriptCodeExtract[checkIndex] != '.'))
			{
				checkIndex--;
			}
			checkIndex--;
			if ((checkIndex > 0) && (scriptCodeExtract[checkIndex] == ']'))
			{
				// character[EGO].Say(); return a pointer to EGO
				while ((checkIndex > 0) && (scriptCodeExtract[checkIndex] != '['))
				{
					checkIndex--;
				}
				checkIndex++;
				return checkIndex;
			}
			while ((checkIndex >= 0) && (Char.IsLetterOrDigit(scriptCodeExtract[checkIndex])))
			{
				checkIndex--;
			}
			checkIndex++;
			return (checkIndex >= 0) ? checkIndex : 0;
		}

		private int FindIndexOfCharacterNameInGlobalFunctionCall(string scriptCodeExtract)
		{
			bool ignoredFirstComma = false;
			for (int checkIndex = scriptCodeExtract.Length - 1; checkIndex >= 0; checkIndex--)
			{
				if ((scriptCodeExtract[checkIndex] == ',') ||
					(scriptCodeExtract[checkIndex] == '('))
				{
					if (!ignoredFirstComma)
					{
						ignoredFirstComma = true;
					}
					else
					{
						checkIndex++;
						while (scriptCodeExtract[checkIndex] == ' ')
						{
							checkIndex++;
						}
						return checkIndex;
					}
				}
			}
			return 0;
		}

		private int ParseNumericalCharacterID(string characterName)
		{
			int i = 0;
			while ((i < characterName.Length) && (Char.IsDigit(characterName[i])))
			{
				i++;
			}
			if (i == characterName.Length)
			{
				int charId = int.Parse(characterName);
				if ((charId >= 0) && (charId < _game.Characters.Count))
				{
					return charId;
				}
			}
			return -1;
		}

	}
}
