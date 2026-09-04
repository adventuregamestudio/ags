using AGS.Types;
using System;
using System.Collections.Generic;
using System.Text;

namespace AGS.Editor
{
    /// <summary>
    /// UpgradeGameTextEncodingTask enforces UTF-8 encoding.
    /// </summary>
    public class UpgradeGameTextEncodingTask : IUpgradeGameTask
    {
        public UpgradeGameTextEncodingTask()
        {
            Enabled = true;
        }

        /// <summary>
        /// A unique string identifier of this upgrade task.
        /// </summary>
        public string ID { get { return "UpgradeGameTextEncodingTask"; } }
        /// <summary>
        /// An arbitrary title, used to identify this task when
        /// presenting to a user.
        /// </summary>
        public string Title { get { return "Convert game to UTF-8 encoding"; } }
        /// <summary>
        /// An arbitrary description, may contain any amount of text.
        /// </summary>
        public string Description
        {
            get
            {
                return "In AGS 4.0 the ASCII game text encoding is no longer supported. Your game will be converted to UTF-8 format; this will affect game scripts and all the text properties." +
                    Environment.NewLine + Environment.NewLine +
                    "If your game has only English texts, then everything will remain as it were." + Environment.NewLine +
                    "If you were using any language(s) other than English in your game, then likely you have ANSI-compatible fonts. In this case the texts in non-English languages will appear \"broken\" in game. This can be fixed by replacing ANSI fonts with Unicode-compatible fonts. This is something that AGS cannot do on its own; you will have to find suitable fonts yourself and import them into the game, replacing existing ones.";
            }
        }
        /// <summary>
        /// A game project version that introduced this upgrade task.
        /// If a loaded game has a less project version, then this task
        /// must be applied, otherwise it should not.
        /// Returns null if should be applied regardless of the game version
        /// (but the execution process may still have version checks inside).
        /// </summary>
        public System.Version GameVersion { get { return null; } }
        /// <summary>
        /// A game project version in form of a numeric index, for the projects
        /// which used these.
        /// </summary>
        public int? GameVersionIndex { get { return null; } }
        /// <summary>
        /// Tells whether this upgrade task is to be executed unconditionally,
        /// without warning user about it.
        /// </summary>
        public bool Implicit { get { return false; } }
        /// <summary>
        /// Tells whether this upgrade task may be disabled by user's choice.
        /// </summary>
        public bool Optional { get { return false; } }
        /// <summary>
        /// Tells whether the upgrade process is allowed to continue if this
        /// task had errors.
        /// </summary>
        public bool AllowToSkipIfHadErrors { get { return false; } }
        /// <summary>
        /// Tells whether user should be asked for a confirmation in order to
        /// continue the upgrade process in case this task had errors.
        /// </summary>
        public bool RequestConfirmationOnErrors { get { return false; } }
        /// <summary>
        /// Tells which stage should this task be run on.
        /// </summary>
        public UpgradeGameTaskStage Stage { get { return UpgradeGameTaskStage.PostStage; } }

        /// <summary>
        /// Whether this task is enabled, otherwise should be skipped.
        /// </summary>
        public bool Enabled { get; set; }

        /// <summary>
        /// Tells whether this task should be applied to this game.
        /// This method can have additional conditions, besides the default version check.
        /// </summary>
        public bool ShouldApplyToGame(Game game)
        {
            return game.Settings.GameTextEncoding != Encoding.UTF8.WebName;
        }

        /// <summary>
        /// Provides WizardPage control(s) used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            return new UpgradeGameWizardPage[] { new UpdateGameGenericInfoPage(game, this) };
        }

        /// <summary>
        /// Apply task options reading them from the dictionary of key-values.
        /// </summary>
        public void ApplyOptions(Dictionary<string, string> options)
        {
            // do nothing
        }

        /// <summary>
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public void Execute(Game game, IWorkProgress progress, CompileMessages errors)
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
