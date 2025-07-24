using System;
using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor
{
    public class UpgradeGameFontsToFontFilesTask : IUpgradeGameTask
    {
        /// <summary>
        /// A unique string identifier of this upgrade task.
        /// </summary>
        public string ID { get { return "UpgradeGameFontsToFontFiles"; } }
        /// <summary>
        /// An arbitrary title, used to identify this task when
        /// presenting to a user.
        /// </summary>
        public string Title { get { return "Separate Fonts into Font Files and Fonts"; } }
        /// <summary>
        /// An arbitrary description, may contain any amount of text.
        /// </summary>
        public string Description
        {
            get
            {
                return "In AGS 4.0 there's a distinction between Font Files and Fonts. Previously you had to add a new font file to your project whenever you needed a font of different size. In 4.0 you can import a single font file, and then create multiple Fonts from it, each with different settings." +
                    Environment.NewLine + Environment.NewLine +
                    "During this upgrade step your project's font files will be moved to a \"Fonts\" folder, and there will be respective Font Files and Font items created in the project tree.";
            }
        }
        /// <summary>
        /// A game project version that introduced this upgrade task.
        /// If a loaded game has a less project version, then this task
        /// must be applied, otherwise it should not.
        /// Returns null if should be applied regardless of the game version
        /// (but the execution process may still have version checks inside).
        /// </summary>
        public System.Version GameVersion { get { return new System.Version(AGSEditor.FIRST_XML_VERSION_USING_INDEX); } }
        /// <summary>
        /// A game project version in form of a numeric index, for the projects
        /// which used these.
        /// </summary>
        public int? GameVersionIndex { get { return AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_FONT_SOURCES; } }
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
        /// Whether this task is enabled, otherwise should be skipped.
        /// </summary>
        public bool Enabled { get; set; }

        /// <summary>
        /// Provides a WizardPage control used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public UpgradeGameWizardPage CreateWizardPage(Game game)
        {
            return new UpdateGameGenericInfoPage(game, this);
        }
        /// <summary>
        /// Apply task options reading them from the dictionary of key-values.
        /// </summary>
        public void ApplyOptions(Dictionary<string, string> options)
        {
            // does not have any options
        }
        /// <summary>
        /// Execute the upgrade task over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            _convertFonts(game, progress, errors);
        }


        // TODO: revise this later.
        // The conversion process is run via a delegate here, because at the time
        // I was not certain if it should remain in FontComponent or not.
        // It sort of makes sense, conceptually.
        // Also, the conversion calls a number of private methods from FontComponent,
        // therefore this seemed to be the most trivial way to proceed.
        public delegate void ConvertFonts(Game game, IWorkProgress progress, CompileMessages errors);
        private ConvertFonts _convertFonts;

        public UpgradeGameFontsToFontFilesTask(ConvertFonts convertFonts)
        {
            _convertFonts = convertFonts;
        }
    }
}
