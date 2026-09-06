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
    public class UpgradeGameIntroAndBackupTask : UpgradeGameTaskBase
    {
        public UpgradeGameIntroAndBackupTask() : base("UpgradeGameIntroAndBackup")
        {
            Title = "Backup project files";
            Implicit = false;
            Optional = true;
            AllowToSkipIfHadErrors = true; // well, not good, but allow this
            RequestConfirmationOnErrors = true; // still must ask user if they like to skip backup
            Stage = UpgradeGameTaskStage.PreStage; // must be complete before any changes done to project
            Enabled = true;
        }

        /// <summary>
        /// A directory to copy backup files to.
        /// </summary>
        public string BackupPath { get; set; }

        /// <summary>
        /// Provides WizardPage controls used to represent this upgrade task.
        /// The page implementation may have this IUpgradeGameTask passed into
        /// constructor in order to assign settings right into it.
        /// </summary>
        public override UpgradeGameWizardPage[] CreateWizardPages(Game game)
        {
            return new UpgradeGameWizardPage[] { new UpgradeGameIntroPage(game, this) };
        }
        /// <summary>
        /// Execute the upgrade step over the given Game project.
        /// Fills any errors or warnings into the provided "errors" collection.
        /// </summary>
        public override void Execute(Game game, IWorkProgress progress, CompileMessages errors)
        {
            if (string.IsNullOrEmpty(BackupPath))
            {
                errors.Add(new CompileError("Invalid backup location"));
                return;
            }

            // CHECKME: not sure if we should backup media resources here
            string[] patternStr = AGSEditor.Instance.Tasks.GetPatternsForStandardGameFiles(game, mediaResources: true);
            var patterns = IncludeUtils.CreatePatternList(patternStr, IncludeUtils.MatchOption.CaseInsensitive);
            string[] filesToBackup = Utilities.GetDirectoryFileList(game.DirectoryPath, "*", SearchOption.AllDirectories, relativePaths: true);
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
