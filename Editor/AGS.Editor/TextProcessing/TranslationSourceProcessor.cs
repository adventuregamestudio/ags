using AGS.Types;
using System;
using System.Collections.Generic;
using System.Linq;

namespace AGS.Editor
{
    public class TranslationSourceProcessor : GameSpeechProcessor
    {
        private Dictionary<string, GameTextLine> _linesProcessed;
        private string _includeScriptPrefix;
        private string _excludeScriptPrefix;
        private string[] _excludeFunctionCalls;

        public TranslationSourceProcessor(Game game, CompileMessages errors) 
            : base(game, errors, makesChanges: false, processHotspotAndObjectDescriptions: true,
                  lookupForFunctionCalls: true, lookupForOuterFunctionCalls: true)
        {
            _includeScriptPrefix = game.Settings.TranslationIncludeScriptPrefix;
            _excludeScriptPrefix = game.Settings.TranslationExcludeScriptPrefix;
            _excludeFunctionCalls = game.Settings.TranslationExcludeFunctionCall.Split(',')
                .Select((s) => s.Trim()).Where((s) => !string.IsNullOrEmpty(s)).ToArray();

            // Only parse function calls if there's a need to exclude any
            LookupForFunctionCalls = _excludeFunctionCalls.Length > 0;

            _linesProcessed = new Dictionary<string, GameTextLine>();
        }

        public ICollection<GameTextLine> LinesForTranslation
        {
            get { return _linesProcessed.Values; }
        }

        protected override bool ParseFunctionCall(string scriptCodeExtract, out int characterID)
        {
            // dummy character ID 0, because we don't really care
            characterID = 0;

            if (string.IsNullOrEmpty(scriptCodeExtract))
                return true; // not a function call, use always

            // Code extract contains "...functionName(" text.
            // Get function name alone and compare to all the excluded function names.
            // TODO: this really should be done by something that returns code extract?
            int nameEndsAt = scriptCodeExtract.Length - 1;
            for (; nameEndsAt >= 0 && (scriptCodeExtract[nameEndsAt] == '(' || char.IsWhiteSpace(scriptCodeExtract[nameEndsAt])); --nameEndsAt);
            int nameStartsAt = nameEndsAt;
            for (; nameStartsAt > 0 && scriptCodeExtract[nameStartsAt - 1].IsScriptWordChar(); --nameStartsAt);
            string functionName = (nameStartsAt >= 0 && nameEndsAt >= 0) ?
                scriptCodeExtract.Substring(nameStartsAt, nameEndsAt - nameStartsAt + 1) : string.Empty;
            
            if (!string.IsNullOrEmpty(functionName))
            {
                foreach (string fn in _excludeFunctionCalls)
                {
                    if (functionName == fn)
                        return false;
                }
            }
            return true;
        }

        protected override string CreateSpeechLine(GameTextLine textLine, GameTextType textType)
        {
            var text = textLine.Text;
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
                        _linesProcessed.Add(text, textLine);
                    }
                }
            }
            return text;
        }
    }
}
