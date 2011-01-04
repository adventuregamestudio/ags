using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class TextImportProcessor : GameSpeechProcessor
    {
        private Dictionary<string, string> _translationToUse;

        public TextImportProcessor(Game game, CompileMessages errors, Dictionary<string, string> translationToUse)
            : base(game, errors, true, true)
        {
            _translationToUse = translationToUse;
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
            if ((_translationToUse.ContainsKey(text)) &&
                (_translationToUse[text].Length > 0))
            {
                return _translationToUse[text];
            }
            return text;
        }

    }
}
