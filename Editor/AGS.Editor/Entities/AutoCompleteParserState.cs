using AGS.Types.AutoComplete;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    // Parser state remembers words that are part of the declaration.
    // They include the name of the symbol, all the modifiers, and qualifiers.
    //
    // For instance:
    // protected static readonly attribute Character * [] IndexedAttribName [];
    // 0         1      2        3         4         5 6  7                 8 
    internal class AutoCompleteParserState
    {
        // We reserve few dummy words in the list, to simplify access to "Previous word"
        private int RESERVED_WORDS = 3;

        public AutoCompleteParserState()
        {
            ClearPreviousWords();
        }

        public void AddNextWord(string word)
        {
            Words.Add(word);
        }

        public void UndoLastWord()
        {
            if (Words.Count > RESERVED_WORDS)
                Words.RemoveAt(Words.Count - 1);
        }

        public bool IsWordInList(string lookForWord)
        {
            return FindWordInList(lookForWord) >= 0;
        }

        public int FindWordInList(string lookForWord)
        {
            for (int i = Words.Count - 1; i >= RESERVED_WORDS; i--)
            {
                if (Words[i] == lookForWord)
                {
                    return i;
                }
            }
            return -1;
        }

        public void ClearPreviousWords()
        {
            Words.Clear();
            // add RESERVED_WORDS dummy slots to simplify "previous word" accessors
            Words.Add(string.Empty);
            Words.Add(string.Empty);
            Words.Add(string.Empty);
        }

        public int WordCount
        {
            get { return Words.Count - RESERVED_WORDS; }
        }

        public string LastWord
        {
            get { return Words[Words.Count - 1]; }
        }

        public string PreviousWord
        {
            get { return Words[Words.Count - 2]; }
        }

        public string PreviousWord2
        {
            get { return Words[Words.Count - 3]; }
        }

        /// <summary>
        /// Returns a word using a backwards index.
        /// </summary>
        public string this[int i]
        {
            get
            {
                return (i < Words.Count - RESERVED_WORDS) ? Words[Words.Count - 1 - i] : string.Empty;
            }
        }

        // TODO: this has to be optimized for fast adding/undoing words during parsing;
        // not certain if List<> is best for this, investigate if there are faster solutions.
        private List<string> Words = new List<string>();
        public string InsideIfNDefBlock = null;
        public string InsideIfDefBlock = null;
        public ScriptEnum InsideEnumDefinition = null;
        public ScriptStruct InsideStructDefinition = null;
        public ScriptFunction InsideFunctionBody = null;
        public int CurrentScriptCharacterIndex;
        public string PreviousComment = null;
    }
}
