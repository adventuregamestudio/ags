using System;
using System.Collections.Generic;
using AGS.Types;

namespace AGS.Editor
{
    public class UpgradeGameFontsToFontFilesTask : UpgradeGameTaskBase
    {
        // TODO: revise this later.
        // The conversion process is run via a delegate here, because at the time
        // I was not certain if it should remain in FontComponent or not.
        // It sort of makes sense, conceptually.
        // Also, the conversion calls a number of private methods from FontComponent,
        // therefore this seemed to be the most trivial way to proceed.
        public delegate void ConvertFonts(Game game, IWorkProgress progress, CompileMessages errors);
        private ConvertFonts _convertFonts;

        public UpgradeGameFontsToFontFilesTask(ConvertFonts convertFonts) : base("UpgradeGameFontsToFontFiles")
        {
            Title = "Separate Fonts into Font Files and Fonts";
            Description = "In AGS 4.0 there's a distinction between Font Files and Fonts. Previously you had to add a new font file to your project whenever you needed a font of different size. In 4.0 you can import a single font file, and then create multiple Fonts from it, each with different settings." +
                    Environment.NewLine + Environment.NewLine +
                    "During this upgrade step your project's font files will be moved to a \"Fonts\" folder, and there will be respective Font Files and Font items created in the project tree.";
            GameVersion = new System.Version(AGSEditor.FIRST_XML_VERSION_USING_INDEX);
            GameVersionIndex = AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_FONT_SOURCES;
            Implicit = false;
            Optional = false;
            AllowToSkipIfHadErrors = false;
            RequestConfirmationOnErrors = false;
            Enabled = true;

            _convertFonts = convertFonts;
        }

        /// <summary>
        /// Provides WizardPage controls used to represent this upgrade task.
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
            _convertFonts(game, progress, errors);
        }
    }
}
