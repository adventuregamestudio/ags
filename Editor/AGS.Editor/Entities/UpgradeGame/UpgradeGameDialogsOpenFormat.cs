using AGS.Types;
using System;

namespace AGS.Editor
{
    public class UpgradeGameDialogsOpenFormat : UpgradeGameTaskBase
    {
        public delegate void ConvertDialogs(Game game, IWorkProgress progress, CompileMessages errors);
        private ConvertDialogs _convertDialogs;

        public UpgradeGameDialogsOpenFormat(ConvertDialogs convertDialogs) : base("UpgradeGameDialogsOpenFormat")
        {
            Title = "Save Dialogs and Dialog Scripts as individual files";
            Description = $"In AGS 4.0 the Dialogs will now be stored separately from the rest of the game data, inside the \"Dialogs\" folder, where each Dialog is saved as a {Dialog.DIALOG_DATA_FILE_EXT.ToUpper()} file, and each Dialog Script is saved as a {DialogScript.DIALOG_SCRIPT_FILE_EXT.ToUpper()} file." +
                    Environment.NewLine +
                    "If a \"Dialogs\" folder already exists, then it will be renamed into a backup folder. You can decide what to do with it on your own later.";
            GameVersion = new System.Version(AGSEditor.AGS_4_0_0_XML_VERSION_OPEN_DIALOGS);
            Implicit = false;
            Optional = false;
            AllowToSkipIfHadErrors = false;
            RequestConfirmationOnErrors = false;
            Enabled = true;

            _convertDialogs = convertDialogs;
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
            _convertDialogs(game, progress, errors);
        }
    }
}
