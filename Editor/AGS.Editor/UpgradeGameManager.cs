using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using AGS.Types;

namespace AGS.Editor
{
    public static class UpgradeGameManager
    {
        public struct UpgradeGameResult
        {
            public bool Success { get; private set; }
            public bool ForceSaveProject { get; private set; }

            public UpgradeGameResult(bool success, bool forceSave)
            {
                Success = success;
                ForceSaveProject = forceSave;
            }
        };

        private static IUpgradeGameTask[] GetAllTasks()
        {
            IUpgradeGameTask[] commonTasks = new IUpgradeGameTask[]
            {
                // add common updates here
                new UpgradeGameCommonTask()
            };

            List<IUpgradeGameTask> tasks = new List<IUpgradeGameTask>(commonTasks);
            Factory.Events.OnGamePrepareUpgrade(new UpgradeGameEventArgs(tasks));
            return tasks.ToArray();
        }

        private static IUpgradeGameTask[] GetTasksForGameVersion(System.Version gameVersion, int? gameVersionIndex)
        {
            var allTasks = GetAllTasks();

            List<IUpgradeGameTask> tasks = new List<IUpgradeGameTask>();
            bool needBackup = false;
            foreach (var task in allTasks)
            {
                if ((task.GameVersion == null) || // if no version is defined, then run always
                    (gameVersion < task.GameVersion) ||
                    (task.GameVersionIndex.HasValue && gameVersionIndex.HasValue && gameVersionIndex < task.GameVersionIndex))
                {
                    tasks.Add(task);
                    // We request Backup task whenever there's a non-implicit upgrade task
                    needBackup |= !task.Implicit;
                }
            }

            // If need a project backup task, always insert one at the beginning of the list
            if (needBackup)
            {
                tasks.Insert(0, new UpgradeGameIntroAndBackupTask());
            }

            return tasks.ToArray();
        }

        private struct ExecuteTasksData
        {
            public Game Game;
            public IUpgradeGameTask[] UpgradeTasks;
            public CompileMessages Errors;
            public bool AbortedOnError;
            public bool CancelledByUser;

            public ExecuteTasksData(Game game, IUpgradeGameTask[] tasks, CompileMessages errors)
            {
                Game = game;
                UpgradeTasks = tasks;
                Errors = errors;
                AbortedOnError = false;
                CancelledByUser = false;
            }
        }

        private static void ExecuteTasksOnBackgroundThread(ExecuteTasksData execData)
        {
            BusyDialog.Show("Please wait while the project update operations are performed...",
                new BusyDialog.ProcessingHandler(ExecuteTasksThread), execData);
        }

        private static object ExecuteTasksThread(IWorkProgress progress, object execDataObject)
        {
            ExecuteTasksData execData = (ExecuteTasksData)execDataObject;
            int current = 0;
            int total = execData.UpgradeTasks.Length;

            foreach (IUpgradeGameTask task in execData.UpgradeTasks)
            {
                progress.SetProgress(total, current, task.Title, autoFormatProgress: true);

                CompileMessages errors = new CompileMessages();
                bool notifiedUserOfError = false;
                try
                {
                    task.Execute(execData.Game, progress, errors);
                }
                catch (Exception ex)
                {
                    // Deal with exception
                    string errorMessage = $"An error occured whilst performing operation: \"{task.Title}\".{Environment.NewLine}{Environment.NewLine}Error: {ex.Message}";
                    if (task.AllowToSkipIfHadErrors)
                    {
                        if (task.RequestConfirmationOnErrors)
                        {
                            if (Factory.GUIController.ShowQuestion(errorMessage +
                                $"{Environment.NewLine}{Environment.NewLine}Do you want to proceed and upgrade the game anyway?", MessageBoxIcon.Warning) == DialogResult.No)
                            {
                                execData.CancelledByUser = true;
                                return null;
                            }
                            notifiedUserOfError = true;
                        }
                        errors.Add(new CompileError($"An error occured whilst performing operation: \"{task.Title}\".\nError: {ex.Message}"));
                    }
                    else
                    {
                        Factory.GUIController.ShowMessage(errorMessage +
                            $"{Environment.NewLine}{Environment.NewLine}The upgrade process will be cancelled.", MessageBoxIcon.Error);
                        execData.AbortedOnError = true;
                        return null;
                    }
                }

                // Deal with the errors which occured during this task
                if (errors.HasErrors)
                {
                    string errorMessage = $"One or more errors have occured whilst performing operation: \"{task.Title}\".{Environment.NewLine}{Environment.NewLine}First error: {errors.FirstError.AsString}";
                    if (task.AllowToSkipIfHadErrors)
                    {
                        if (!notifiedUserOfError && task.RequestConfirmationOnErrors)
                        {
                            if (Factory.GUIController.ShowQuestion(errorMessage +
                                $"{Environment.NewLine}{Environment.NewLine}Do you want to proceed and upgrade the game anyway?", MessageBoxIcon.Warning) == DialogResult.No)
                            {
                                execData.CancelledByUser = true;
                                return null;
                            }
                        }
                    }
                    else
                    {
                        Factory.GUIController.ShowMessage(errorMessage +
                            $"{Environment.NewLine}{Environment.NewLine}The upgrade process will be cancelled.", MessageBoxIcon.Error);
                        execData.AbortedOnError = true;
                        return null;
                    }
                }

                // Still running? Merge messages and go to the next task
                execData.Errors.AddRange(errors);
                current++;
            }

            return null; // we do not return anything (the upgraded game is saved in exec data)
        }

        /// <summary>
        /// Show Upgrade Game Wizard dialog, containing pages for each of the input tasks.
        /// Returns a new array of tasks generated after user have passed through the wizard.
        /// This array may be equal to the input array, or be a new one, containing less tasks,
        /// in case user ticked some tasks out (those that permit disabling themselves).
        /// If user cancelled the wizard, then returns null instead.
        /// </summary>
        private static IUpgradeGameTask[] ShowWizard(Game game, IUpgradeGameTask[] tasks)
        {
            List<WizardPage> pages = new List<WizardPage>();
            foreach (var task in tasks)
            {
                if (!task.Implicit)
                {
                    var pagesForTask = task.CreateWizardPages(game);
                    if (pagesForTask != null)
                    {
                        pages.AddRange(pagesForTask);
                    }
                }
            }

            string oldVersion = game.SavedXmlEditorVersion != null ?
                $"AGS {game.SavedXmlEditorVersion}" : "a much older version";
            WizardDialog dialog = new WizardDialog("Upgrade Game Project",
                $"You are about to upgrade your game from {oldVersion} to AGS {AGS.Types.Version.AGS_EDITOR_VERSION}.{Environment.NewLine}{Environment.NewLine}" +
                $"This wizard will guide you through upgrading process, and will suggest you to make a backup copy of your project first, where you are free to either accept or ignore this suggestion.{Environment.NewLine}{Environment.NewLine}" +
                $"Each page in this wizard will notify you about a serious change that your project will undergo. Most of these notifications are purely informational and their purpose is to help you understand how your project files will be altered. Others may require you to make a choice between two or more ways of applying certain change, or not applying at all.{Environment.NewLine}{Environment.NewLine}" +
                "You may cancel the upgrade anytime before pressing on Finish button. If you cancel the upgrade, no changes will be done to your project."
                , pages);
            DialogResult result = dialog.ShowDialog();
            bool doUpgrade = (result == DialogResult.OK);
            dialog.Dispose();
            if (!doUpgrade)
                return null;

            // Gather final tasks, which were not disabled by the user selection in wizard dialog
            return tasks.Where(t => t.Enabled).ToArray();
        }

        public static bool UpgradeWithoutWizard(Game game, Dictionary<string, string> options, CompileMessages errors, out UpgradeGameResult result)
        {
            var tasks = GetTasksForGameVersion(game.SavedXmlVersion, game.SavedXmlVersionIndex);
            bool hasExplicitTasks = tasks.FirstOrDefault(task => !task.Implicit) != null;
            // TODO: apply options to Tasks
            var execData = new ExecuteTasksData(game, tasks, errors);
            ExecuteTasksOnBackgroundThread(execData);
            result = new UpgradeGameResult(!execData.AbortedOnError && !execData.CancelledByUser, hasExplicitTasks);
            return result.Success;
        }

        public static bool UpgradeWithWizard(Game game, CompileMessages errors, out UpgradeGameResult result)
        {
            var tasks = GetTasksForGameVersion(game.SavedXmlVersion, game.SavedXmlVersionIndex);
            // No tasks counts as auto-success, where we skip upgrading and end opening the project
            if (tasks.Length == 0)
            {
                result = new UpgradeGameResult(success: true, forceSave: false);
                return true;
            }

            bool hasExplicitTasks = tasks.FirstOrDefault(task => !task.Implicit) != null;
            bool mustShowWizard = hasExplicitTasks;

            // If there are tasks that require user attention, then gather pages
            if (mustShowWizard)
            {
                var finalTasks = ShowWizard(game, tasks);
                // We only cancel if the returned collection is null,
                // but it's allowed to be empty, in which case we assume that project should
                // be opened, but no upgrade tasks are necessary.
                if (finalTasks == null)
                {
                    result = new UpgradeGameResult(success: false, forceSave: false);
                    return false;
                }
                else if (finalTasks.Length == 0)
                {
                    result = new UpgradeGameResult(success: true, forceSave: false);
                    return true;
                }
                tasks = finalTasks;
            }

            var execData = new ExecuteTasksData(game, tasks, errors);
            ExecuteTasksOnBackgroundThread(execData);
            result = new UpgradeGameResult(!execData.AbortedOnError && !execData.CancelledByUser, hasExplicitTasks);

            if (errors.HasErrorsOrWarnings)
            {
                Factory.GUIController.ShowMessage("There were errors or warnings when upgrading the game project. Please consult the output window for details.", MessageBoxIcon.Warning);
            }

            return result.Success;
        }
    }
}
