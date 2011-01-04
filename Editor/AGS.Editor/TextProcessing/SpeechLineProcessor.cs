using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class SpeechLineProcessor : SpeechOnlyProcessor
    {
        private Dictionary<int, Dictionary<string, string>> _existingLines;
        private Dictionary<int, int> _speechLineCount;
        private bool _combineIdenticalLines;
        private bool _includeNarrator;
        private bool _removeNumbering;
        private int? _characterID;
        private StreamWriter _referenceFile;

        public SpeechLineProcessor(Game game, bool includeNarrator, bool combineIdenticalLines, 
            bool removeNumbering, int? characterID,
            Dictionary<string, FunctionCallType> speechableFunctionCalls, 
            CompileMessages errors, StreamWriter referenceFile) :
            base(game, errors, true, false, speechableFunctionCalls)
        {
            _speechLineCount = new Dictionary<int, int>();
            _combineIdenticalLines = combineIdenticalLines;
            _includeNarrator = includeNarrator;
            _referenceFile = referenceFile;
            _removeNumbering = removeNumbering;
            _characterID = characterID;

            if (combineIdenticalLines)
            {
                _existingLines = new Dictionary<int, Dictionary<string, string>>();
            }
        }

        protected override string CreateSpeechLine(int speakingCharacter, string text)
        {
            if ((speakingCharacter == Character.NARRATOR_CHARACTER_ID) && (!_includeNarrator))
            {
                return text;
            }
            if (speakingCharacter < 0)
            {
                return text;
            }
            if ((_characterID.HasValue) && (speakingCharacter != _characterID.Value))
            {
                return text;
            }
            if (text == "...")
            {
                return text;
            }

            if (text.StartsWith("&"))
            {
                text = text.Substring(1);
                while ((text.Length > 0) && (Char.IsDigit(text[0])))
                {
                    text = text.Substring(1);
                }
                text = text.Trim();
            }

            if (_combineIdenticalLines)
            {
                if (_existingLines.ContainsKey(speakingCharacter))
                {
                    if (_existingLines[speakingCharacter].ContainsKey(text))
                    {
                        return _existingLines[speakingCharacter][text];
                    }
                }
            }

            if (_speechLineCount.ContainsKey(speakingCharacter))
            {
                _speechLineCount[speakingCharacter]++;
            }
            else
            {
                _speechLineCount.Add(speakingCharacter, 1);
            }

            string lineWithNewToken;
            if (_removeNumbering)
            {
                lineWithNewToken = text;
            }
            else
            {
                lineWithNewToken = string.Format("&{0} {1}", _speechLineCount[speakingCharacter], text);
            }

            string charName = (speakingCharacter == Character.NARRATOR_CHARACTER_ID) ? NARRATOR_NAME : _game.Characters[speakingCharacter].ScriptName;
            _referenceFile.WriteLine(charName + ": " + lineWithNewToken);

            if (_combineIdenticalLines)
            {
                if (!_existingLines.ContainsKey(speakingCharacter))
                {
                    _existingLines.Add(speakingCharacter, new Dictionary<string, string>());
                }
                _existingLines[speakingCharacter].Add(text, lineWithNewToken);
            }

            return lineWithNewToken;
        }
    }
}
