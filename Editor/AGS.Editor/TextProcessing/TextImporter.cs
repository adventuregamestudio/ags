using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    public class TextImporter
    {
        public static CompileMessages ReplaceAllGameText(Game game, Translation withTranslation)
        {
            CompileMessages errors = new CompileMessages();

            TextImportProcessor processor = new TextImportProcessor(game, errors, withTranslation.TranslatedLines);

            TextProcessingHelper.ProcessAllGameText(processor, game, errors);

            return errors;
        }
    }
}
