using AGS.Types;
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class OldGameImporter
    {
        public Game ImportGameFromAGS272(string gameToLoad, bool useWizard)
        {
            string backupLocation = ConstructBackupDirectoryName(gameToLoad);
            Game game = null;
            bool continueWithImport = true;
            bool performBackup = false;

            if (useWizard)
            {
                ImportGameWizardPage importPage = new ImportGameWizardPage(backupLocation);
                List<WizardPage> pages = new List<WizardPage>();
                pages.Add(importPage);
                WizardDialog dialog = new WizardDialog("Import Old Game", "This wizard will guide you through importing a game from a previous version of AGS.", pages);
                DialogResult result = dialog.ShowDialog();
                continueWithImport = (result == DialogResult.OK);
                performBackup = importPage.BackupEnabled;
                dialog.Dispose();
            }

            if (continueWithImport)
            {
                if (performBackup)
                {
                    try
                    {
                        BusyDialog.Show("Please wait while your game files are backed up...", new BusyDialog.ProcessingHandler(MakeBackupCopyOfGameFolderThread), backupLocation);
                    }
                    catch (Exception ex)
                    {
                        if (Factory.GUIController.ShowQuestion("An error occured whilst backing up your game files.\n\nError: " + ex.Message + "\n\nDo you want to proceed and upgrade the game anyway?", MessageBoxIcon.Warning) == DialogResult.No)
                        {
                            return null;
                        }
                    }
                }
                ImportGameResult importResult = (ImportGameResult)BusyDialog.Show("Please wait while your game is imported...", new BusyDialog.ProcessingHandler(ImportOldGameThread), gameToLoad);
                game = importResult.LoadedGame;
                Factory.Events.OnImportedOldGame();

                Factory.GUIController.ShowOutputPanel(importResult.Errors);
                if (importResult.Errors.Count > 0)
                {
                    Factory.GUIController.ShowMessage("Some errors were encountered whilst importing the game. Check the output window for details.", MessageBoxIcon.Warning);
                }

            }
            return game;
        }

        private string ConstructBackupDirectoryName(string gameImportPath)
        {
            string backupLocation;
            string rootPath = Path.GetDirectoryName(gameImportPath);
            int attempt = 0;
            do
            {
                backupLocation = Path.Combine(rootPath, "Backup" + (attempt == 0 ? "" : attempt.ToString()));
                attempt++;
            }
            while (Directory.Exists(backupLocation));

            return backupLocation;
        }

        private object ImportOldGameThread(object parameter)
        {
            ImportGameResult result = new ImportGameResult();
            string gameToLoad = (string)parameter;
            result.LoadedGame = Factory.NativeProxy.ImportOldGame(gameToLoad);
            Dictionary<int, Sprite> spriteList = Factory.NativeProxy.LoadSpriteDimensions();
            result.Errors = ImportExport.ImportOldEditorDatFile(gameToLoad, result.LoadedGame, spriteList);
            result.LoadedGame.ConvertCoordinatesToNativeResolution();
            ImportExport.CreateInteractionScripts(result.LoadedGame, result.Errors);
            return result;
        }

        private object MakeBackupCopyOfGameFolderThread(object backupLocationAsObject)
        {
            string backupLocation = (string)backupLocationAsObject;
            string sourceDir = Path.GetDirectoryName(backupLocation);

            string[] fileNames = Utilities.GetDirectoryFileList(sourceDir, "*", SearchOption.AllDirectories);
            foreach (string file in fileNames)
            {
                string destinationFile = backupLocation + file.Substring(sourceDir.Length);
                string destinationDirectory = Path.GetDirectoryName(destinationFile);
                if (!Directory.Exists(destinationDirectory))
                {
                    Directory.CreateDirectory(destinationDirectory);
                }
                File.Copy(file, destinationFile);
            }

            return null;
        }

        private struct ImportGameResult
        {
            internal Game LoadedGame;
            internal CompileMessages Errors;
        }
    }
}
