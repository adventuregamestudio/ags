using AGS.Types.AutoComplete;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    internal class AutoCompleteParserState
    {
        public AutoCompleteParserState()
        {
            for (int i = 0; i < PreviousWords.Length; i++)
            {
                PreviousWords[i] = string.Empty;
            }
        }

        public void AddNextWord(string word)
        {
            for (int i = PreviousWords.Length - 2; i >= 0; i--)
            {
                PreviousWords[i + 1] = PreviousWords[i];
            }
            PreviousWords[0] = word;
        }

        public void UndoLastWord()
        {
            for (int i = 0; i < PreviousWords.Length - 1; i++)
            {
                PreviousWords[i] = PreviousWords[i + 1];
            }
            PreviousWords[PreviousWords.Length - 1] = string.Empty;
        }

        public bool IsWordInPreviousList(string lookForWord)
        {
            for (int i = 0; i < PreviousWords.Length; i++)
            {
                if (PreviousWords[i] == lookForWord)
                {
                    return true;
                }
            }
            return false;
        }

        public void ClearPreviousWords()
        {
            for (int i = 0; i < PreviousWords.Length; i++)
            {
                PreviousWords[i] = string.Empty;
            }
        }

        public string LastWord
        {
            get { return PreviousWords[0]; }
        }

        public string WordBeforeLast
        {
            get { return PreviousWords[1]; }
        }

        public string WordBeforeWordBeforeLast
        {
            get { return PreviousWords[2]; }
        }

        public string[] PreviousWords = new string[6];
        public string InsideIfNDefBlock = null;
        public string InsideIfDefBlock = null;
        public ScriptEnum InsideEnumDefinition = null;
        public ScriptStruct InsideStructDefinition = null;
        public ScriptFunction InsideFunctionBody = null;
        public int CurrentScriptCharacterIndex;
		public bool DynamicArrayDefinition = false;
        public string PreviousComment = null;
    }
}
