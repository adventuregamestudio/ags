using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    /// <summary>
    /// UpgradeGameTextEncodingTask enforces UTF-8 encoding.
    /// </summary>
    public class UpgradeGameTextEncodingTask : UpgradeGameTaskBase
    {
        public UpgradeGameTextEncodingTask() : base("UpgradeGameTextEncodingTask")
        {
            Title = "Convert game to UTF-8 encoding";
            Description = "In AGS 4.0 the ASCII game text encoding is no longer supported. Your game will be converted to UTF-8 format; this will affect game scripts and all the text properties." +
                    Environment.NewLine + Environment.NewLine +
                    "If your game has only English texts, then everything will remain as it were." + Environment.NewLine +
                    "If you were using any language(s) other than English in your game, then likely you have ANSI-compatible fonts. In this case the texts in non-English languages will appear \"broken\" in game. This can be fixed by replacing ANSI fonts with Unicode-compatible fonts. This is something that AGS cannot do on its own; you will have to find suitable fonts yourself and import them into the game, replacing existing ones.";
            Implicit = false;
            Optional = false;
            AllowToSkipIfHadErrors = false;
            RequestConfirmationOnErrors = false;
            Stage = UpgradeGameTaskStage.PostStage; // should do this after any project format conversions
            Enabled = true;
        }

        /// <summary>
        /// Tells whether this task should be applied to this game.
        /// This method can have additional conditions, besides the default version check.
        /// </summary>
        public override bool ShouldApplyToGame(Game game)
        {
            return game.Settings.GameTextEncoding != Encoding.UTF8.WebName;
        }

        /// <summary>
        /// Provides WizardPage control(s) used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public override UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            return new UpgradeGameWizardPage[] { new UpdateGameGenericInfoPage(game, this) };
        }

        /// <summary>
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public override void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            var oldEncoding = game.TextEncoding;
            game.Settings.GameTextEncoding = Encoding.UTF8.WebName;
            Factory.AGSEditor.Tasks.ConvertAllGameTextsNoSaveGame(game,
                            oldEncoding,
                            progress,
                            errors);
            errors.Add(new CompileInformation($"Converted game to UTF-8 text format"));
        }
    }
}
