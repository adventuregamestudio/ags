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

        protected override bool ParseFunctionCall(string scriptCodeExtract, out int characterID)
        {
            // dummy character ID 0, because we don't really care
            characterID = 0;
            return true;
        }

        protected override string CreateSpeechLine(int speakingCharacter, string text, GameTextType textType)
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
