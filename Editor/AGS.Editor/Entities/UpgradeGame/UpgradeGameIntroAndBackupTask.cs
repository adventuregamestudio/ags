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
        public UpgradeGameIntroAndBackupTask()
        {
            Enabled = true;
        }

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
        /// Provides WizardPage controls used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            return new UpgradeGameWizardPage[] { new UpgradeGameIntroPage(game, this) };
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

            string[] includeStr =
            {
                "*",
                "!*/_Debug/",
                "!*/Compiled/",
            };
            var patterns = IncludeUtils.CreatePatternList(includeStr, IncludeUtils.MatchOption.CaseInsensitive);
            string[] filesToBackup = Utilities.GetDirectoryFileList(game.DirectoryPath, "*", SearchOption.AllDirectories);
            filesToBackup = IncludeUtils.FilterItemList(filesToBackup, patterns, IncludeUtils.MatchOption.CaseInsensitive);

            foreach (string file in filesToBackup)
            {
                string relativeFile = Utilities.GetRelativeToProjectPath(file);
                string destinationFile = Path.Combine(BackupPath, relativeFile);
                string destinationDirectory = Path.GetDirectoryName(destinationFile);
                if (!Directory.Exists(destinationDirectory))
                {
                    Directory.CreateDirectory(destinationDirectory);
                }
                File.Copy(file, destinationFile);
            }

            errors.Add(new CompileInformation($"Original game files are backed up in {BackupPath}"));
        }
    }
}
