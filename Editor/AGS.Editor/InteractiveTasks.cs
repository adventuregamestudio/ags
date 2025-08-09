using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class InteractiveTasks
    {
        private const int ENGINE_EXIT_CODE_NORMAL = 0;
        private const int ENGINE_EXIT_CODE_CRASH  = 92;
        private const int ENGINE_EXIT_CODE_ERROR  = 93;

        private delegate void TestGameFinishedDelegate(int exitCode);
        public event Action TestGameFinished;
        public event Action TestGameStarting;

        private Control _mainGUIThread;
        private bool _currentlyTesting = false;
        private Tasks _tasks;

        public InteractiveTasks(Tasks tasks)
        {
            _tasks = tasks;
            _tasks.TestGameFinished += Tasks_TestGameFinished;
        }

        public static void ReportTaskException(string errorMsg, Exception ex)
        {
            string messageDetails = string.Empty;
            if ((!(ex is AGS.Types.InvalidDataException)) &&
                (!(ex is AGS.Types.AGSEditorException)))
            {
                messageDetails = "Error details: " + ex.ToString();
            }
            else if (ex is AGS.Types.AGSEditorException && ex.InnerException != null)
            {
                messageDetails = "Error details: ";
                for (Exception ie = ex.InnerException; ie != null; ie = ie.InnerException)
                    messageDetails += ie.Message + Environment.NewLine + Environment.NewLine;
            }
            Factory.GUIController.ShowMessage(errorMsg + " The error was:" + Environment.NewLine + Environment.NewLine + ex.Message +
                Environment.NewLine + Environment.NewLine + "If you cannot resolve the error, please post on the AGS Technical Forum for assistance." +
                Environment.NewLine + Environment.NewLine + messageDetails, MessageBoxIcon.Warning);
        }

        public bool BrowseForAndLoadGame()
        {
            string gameToLoad = Factory.GUIController.ShowOpenFileDialog("Select game to open", "AGS game files (*.agf)|*.agf", false);
            return LoadGame(gameToLoad);
        }

        public bool LoadGame(string gameToLoad)
        {
            bool loadedSuccessfully = false;            
            if (gameToLoad != null)
            {
                try
                {
                    loadedSuccessfully = LoadGameFromDisk(gameToLoad);
                }
                catch (AGS.Types.InvalidDataException e)
                {
                    Factory.GUIController.ShowMessage("Unable to import the game.\n\n" + e.Message, MessageBoxIcon.Warning);
                }
                catch (AGS.Types.AGSEditorException e)
                {
                    Factory.GUIController.ShowMessage("Unable to import the game.\n\n" + e.Message, MessageBoxIcon.Warning);
                }
            }
            return loadedSuccessfully;
        }

        public bool LoadGameFromDisk(string gameToLoad)
        {
            try
            {
                bool success = _tasks.LoadGameFromDisk(gameToLoad, true);
                AGS.Types.Game game = Factory.AGSEditor.CurrentGame;
                if ((game.SavedXmlVersion != null) && (game.SavedXmlVersion < new System.Version(AGSEditor.LATEST_XML_VERSION)))
                {
                    Factory.GUIController.ShowMessage("This game was last saved with " +
                        ((game.SavedXmlEditorVersion == null) ? "an older version" : ("version " + game.SavedXmlEditorVersion))
                        + " of AGS. If you save it now, the game will be upgraded and previous versions of AGS will be unable to open it.", MessageBoxIcon.Information);
                    game.WorkspaceState.RequiresRebuild = true;
                }
                return success;
            }
            catch (Exception ex)
            {
                ReportTaskException("An error occurred whilst trying to load your game.", ex);
                return false;
            }
        }

        public void CreateTemplateFromCurrentGame(string templateFileName)
        {
            BusyDialog.Show("Please wait while the template is created...", new BusyDialog.ProcessingHandler(CreateTemplateFromCurrentGameProcess), templateFileName);
        }

        private object CreateTemplateFromCurrentGameProcess(IWorkProgress progress, object templateFileName)
        {
            _tasks.CreateTemplateFromCurrentGame((string)templateFileName);
            return null;
        }

        public void TestGame(bool withDebugger)
        {
            _mainGUIThread = new Control();
            IntPtr forceWindowHandleCreation = _mainGUIThread.Handle;
            _currentlyTesting = true;
            try
            {
                _tasks.TestGame(withDebugger);
            }
            catch (Exception ex)
            {
                Tasks_TestGameFinished(-1);
                throw new Exception(ex.Message, ex);
            }

            if (TestGameStarting != null)
            {
                TestGameStarting();
            }
        }

        private void Tasks_TestGameFinished(int exitCode)
        {
            if (_currentlyTesting)
            {
                _currentlyTesting = false;
                _mainGUIThread.Invoke(new TestGameFinishedDelegate(TestGameExitedOnGUIThread), exitCode);
            }
        }

        private void TestGameExitedOnGUIThread(int exitCode)
        {
            if (exitCode == ENGINE_EXIT_CODE_NORMAL)
            {
                // TODO: Check warnings.log and display
            }
            else if (exitCode == ENGINE_EXIT_CODE_CRASH || exitCode == ENGINE_EXIT_CODE_ERROR)
            {
            }
            else
            {
                Factory.GUIController.ShowMessage("The game engine does not appear to have shut down properly. If the problem persists, post the problem on the Tech forum.", MessageBoxIcon.Warning);
            }

            if (TestGameFinished != null)
            {
                TestGameFinished();
            }

        }

    }
}
