using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class TranslationSourceProcessor : GameSpeechProcessor
    {
        private Dictionary<string, string> _linesProcessed;

        public TranslationSourceProcessor(Game game, CompileMessages errors) 
            : base(game, errors, false, true)
        {
            _linesProcessed = new Dictionary<string, string>();
        }

        public ICollection<string> LinesForTranslation
        {
            get { return _linesProcessed.Keys; }
        }

        protected override int ParseFunctionCallAndFindCharacterID(string scriptCodeExtract)
        {
            string[] doNotTranslateCalls = new string[] { "Property(", "Property (", "PropertyText(", "PropertyText (" };
            foreach (string doNotTranslateFunc in doNotTranslateCalls)
            {
                if (scriptCodeExtract.IndexOf(doNotTranslateFunc) > 0)
                {
                    return -1;
                }
            }
            // dummy character ID 0, because we don't really care
            return 0;
        }

        protected override string CreateSpeechLine(int speakingCharacter, string text)
        {
            // ignore blank strings and any that start with // (since they
            // conflict with comments in the translation file)
            if ((text.Trim().Length > 0) && (!text.StartsWith("//")))
            {
                if (!_linesProcessed.ContainsKey(text))
                {
                    _linesProcessed.Add(text, text);
                }
            }
            return text;
        }

    }
}
