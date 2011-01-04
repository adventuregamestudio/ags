using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace AGS.Editor
{
    public class TranslationGenerator
    {
        private ICollection<string> _linesForTranslation;

        public ICollection<string> LinesForTranslation
        {
            get { return _linesForTranslation; }
        }

        public CompileMessages CreateTranslationList(Game game)
        {
            CompileMessages errors = new CompileMessages();

            TranslationSourceProcessor processor = new TranslationSourceProcessor(game, errors);

            TextProcessingHelper.ProcessAllGameText(processor, game, errors);

            _linesForTranslation = processor.LinesForTranslation;

            return errors;
        }
    }
}
