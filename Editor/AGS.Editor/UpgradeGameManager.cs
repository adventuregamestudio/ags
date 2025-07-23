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
                // TODO: add backup task here
                //tasks.Insert(0, ...);
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

            foreach (IUpgradeGameTask task in execData.UpgradeTasks)
            {
                // FIXME: tasks might change the message and counter, so reset the total here too
                // We need to support sub-tasks in IWorkProgress interface and respective implementation!
                progress.Total = execData.UpgradeTasks.Length;
                progress.Current = current;
                progress.Message = task.Title;

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
            Dictionary<WizardPage, IUpgradeGameTask> pageToTask = new Dictionary<WizardPage, IUpgradeGameTask>();
            foreach (var task in tasks)
            {
                if (!task.Implicit)
                {
                    var page = task.CreateWizardPage(game);
                    if (page != null)
                    {
                        pages.Add(page);
                        pageToTask.Add(page, task);
                    }
                }
            }

            WizardDialog dialog = new WizardDialog("Upgrade Game Project",
                $"This wizard will guide you through upgrading a game project from an older version of AGS.{Environment.NewLine}{Environment.NewLine}" +
                $"The wizard will suggest you to make a backup copy of your project first, and you are free to either accept or ignore this suggestion.{Environment.NewLine}{Environment.NewLine}" +
                $"Each page in this wizard will notify you about a serious change that your project will undergo. Most of these notifications are purely informational and their purpose is to help you understand how your project files will be altered. Others may require you to make a choice between two or more ways of applying certain change, or not applying at all.{Environment.NewLine}{Environment.NewLine}" +
                "You may cancel the upgrade anytime before pressing on Finish button. If you cancel the upgrade, no changes will be done to your project."
                , pages);
            DialogResult result = dialog.ShowDialog();
            bool doUpgrade = (result == DialogResult.OK);
            dialog.Dispose();
            if (!doUpgrade)
                return null;

            // For the final tasks, first copy the implicit tasks that do not have respective wizard pages
            List<IUpgradeGameTask> finalTasks = new List<IUpgradeGameTask>();
            finalTasks.AddRange(tasks.Where(t => t.Implicit));

            // Gather enabled explicit tasks
            foreach (var page in pages)
            {
                var upgradePage = (page as UpgradeGameWizardPage);
                if (upgradePage == null)
                    continue;
                if (!upgradePage.Enabled)
                    continue;
                var finalTask = pageToTask[upgradePage];
                finalTasks.Add(finalTask);
            }
            return finalTasks.ToArray();
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
            bool hasExplicitTasks = tasks.FirstOrDefault(task => !task.Implicit) != null;
            bool mustShowWizard = hasExplicitTasks;

            // If there are tasks that require user attention, then gather pages
            if (mustShowWizard)
            {
                var finalTasks = ShowWizard(game, tasks);
                if (finalTasks == null)
                {
                    result = new UpgradeGameResult(false, false);
                    return false;
                }
                tasks = finalTasks;
            }

            var execData = new ExecuteTasksData(game, tasks, errors);
            ExecuteTasksOnBackgroundThread(execData);
            result = new UpgradeGameResult(!execData.AbortedOnError && !execData.CancelledByUser, hasExplicitTasks);

            if (errors.Count > 0)
            {
                Factory.GUIController.ShowMessage("There were errors or warnings when upgrading the game project. Please consult the output window for details.", MessageBoxIcon.Warning);
            }

            return result.Success;
        }
    }
}
