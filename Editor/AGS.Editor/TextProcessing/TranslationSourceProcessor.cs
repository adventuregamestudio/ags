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
        private string _includeScriptPrefix;
        private string _excludeScriptPrefix;

        public TranslationSourceProcessor(Game game, CompileMessages errors) 
            : base(game, errors, false, true)
        {
            _includeScriptPrefix = game.Settings.TranslationIncludeScriptPrefix;
            _excludeScriptPrefix = game.Settings.TranslationExcludeScriptPrefix;

            _linesProcessed = new Dictionary<string, string>();
        }

        public ICollection<string> LinesForTranslation
        {
            get { return _linesProcessed.Keys; }
        }

        protected override int ParseFunctionCallAndFindCharacterID(string scriptCodeExtract)
        {
            // dummy character ID 0, because we don't really care
            return 0;
        }

        protected override string CreateSpeechLine(int speakingCharacter, string text, GameTextType textType)
        {
            // ignore blank strings and any that start with // (since they
            // conflict with comments in the translation file)
            if (!string.IsNullOrWhiteSpace(text) && (!text.StartsWith("//")))
            {
                bool include = true;
                if (textType == GameTextType.Script || textType == GameTextType.DialogScript)
                {
                    if (((_includeScriptPrefix != string.Empty) && !text.StartsWith(_includeScriptPrefix))
                        || ((_excludeScriptPrefix != string.Empty) && text.StartsWith(_excludeScriptPrefix)))
                    {
                        include = false;
                    }
                }

                if (include)
                {
                    if (!_linesProcessed.ContainsKey(text))
                    {
                        _linesProcessed.Add(text, text);
                    }
                }
            }
            return text;
        }
    }
}
