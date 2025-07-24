using System;
using System.Collections.Generic;
using System.IO;
using AGS.Editor.Components;
using AGS.Types;

namespace AGS.Editor
{
    /// <summary>
    /// UpgradeGameIntroAndBackupTask performs an optional project backup;
    /// is supposed to be executed prior to any other upgrade tasks.
    /// </summary>
    public class UpgradeGameIntroAndBackupTask : IUpgradeGameTask
    {
        /// <summary>
        /// A unique string identifier of this upgrade step.
        /// </summary>
        public string ID { get { return "UpgradeGameIntroAndBackup"; } }
        /// <summary>
        /// An arbitrary title, used to identify this step when
        /// presenting to a user.
        /// </summary>
        public string Title { get { return "Backup project files"; } }
        /// <summary>
        /// An arbitrary description, may contain any amount of text.
        /// </summary>
        public string Description { get { return ""; /* TODO? */ } }
        /// <summary>
        /// A game project version that introduced this upgrade step.
        /// If a loaded game has a less project version, then this step
        /// must be applied, otherwise it should not.
        /// </summary>
        public System.Version GameVersion { get { return null; /* any version where needed */ } }
        /// <summary>
        /// A game project version in form of a numeric index, for the projects
        /// which used these.
        /// </summary>
        public int? GameVersionIndex { get { return null; } }
        /// <summary>
        /// Tells whether this upgrade step is to be executed unconditionally,
        /// without warning user about it.
        /// </summary>
        public bool Implicit { get { return false; } }
        /// <summary>
        /// Tells whether this upgrade step may be disabled by user's choice.
        /// </summary>
        public bool Optional { get { return true; } }
        /// <summary>
        /// Tells whether the upgrade process is allowed to continue if this
        /// step had errors.
        /// </summary>
        public bool AllowToSkipIfHadErrors { get { return true; } }
        /// <summary>
        /// Tells whether user should be asked for a confirmation in order to
        /// continue the upgrade process in case this step had errors.
        /// </summary>
        public bool RequestConfirmationOnErrors { get { return true; } }

        /// <summary>
        /// Whether this task is enabled, otherwise should be skipped.
        /// </summary>
        public bool Enabled { get; set; }

        /// <summary>
        /// A directory to copy backup files to.
        /// </summary>
        public string BackupPath { get; set; }

        /// <summary>
        /// Provides a WizardPage control used to represent this upgrade step.
        /// </summary>
        public UpgradeGameWizardPage CreateWizardPage(Game game)
        {
            return new UpgradeGameIntroPage(game, this);
        }
        /// <summary>
        /// Apply task options reading them from the dictionary of key-values.
        /// </summary>
        public void ApplyOptions(Dictionary<string, string> options)
        {

        }
        /// <summary>
        /// Execute the upgrade step over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            if (string.IsNullOrEmpty(BackupPath))
            {
                errors.Add(new CompileError("Invalid backup location"));
                return;
            }
            List<string> filesToBackup = new List<string>();
            Utilities.AddAllMatchingFiles(filesToBackup, AGSEditor.GAME_FILE_NAME);
            Utilities.AddAllMatchingFiles(filesToBackup, AGSEditor.SPRITE_FILE_NAME);
            Utilities.AddAllMatchingFiles(filesToBackup, AGSEditor.SPRITE_INDEX_FILE_NAME);
            Utilities.AddAllMatchingFiles(filesToBackup, "*.asc");
            Utilities.AddAllMatchingFiles(filesToBackup, "*.ash");
            if (game.SavedXmlVersionIndex != null && game.SavedXmlVersionIndex < AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_OPEN_ROOMS)
            {
                Utilities.AddAllMatchingFiles(filesToBackup, "*.crm");
            }
            else
            {
                Utilities.AddAllMatchingFiles(filesToBackup, "Rooms\\*\\data.xml", true);
                Utilities.AddAllMatchingFiles(filesToBackup, "Rooms\\*\\*.png", true);
                Utilities.AddAllMatchingFiles(filesToBackup, "Rooms\\*\\room*.asc", true);
            }
            if (game.SavedXmlVersionIndex != null && game.SavedXmlVersionIndex < AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_FONT_SOURCES)
            {
                Utilities.AddAllMatchingFiles(filesToBackup, "*.ttf");
                Utilities.AddAllMatchingFiles(filesToBackup, "*.wfn");
            }
            else
            {
                Utilities.AddAllMatchingFiles(filesToBackup, FontsComponent.FONT_FILES_DIRECTORY, "*.ttf", true);
                Utilities.AddAllMatchingFiles(filesToBackup, FontsComponent.FONT_FILES_DIRECTORY, "*.wfn", true);
            }
            Utilities.AddAllMatchingFiles(filesToBackup, "*.trs");

            foreach (string file in filesToBackup)
            {
                string destinationFile = Path.Combine(BackupPath, file);
                string destinationDirectory = Path.GetDirectoryName(destinationFile);
                if (!Directory.Exists(destinationDirectory))
                {
                    Directory.CreateDirectory(destinationDirectory);
                }
                File.Copy(file, destinationFile);
            }
        }
    }
}
