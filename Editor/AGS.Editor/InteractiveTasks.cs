using System;
using System.Collections.Generic;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class InteractiveTasks
    {
        private const int ENGINE_EXIT_CODE_NORMAL = 91;
        private const int ENGINE_EXIT_CODE_CRASH = 92;

        private delegate void TestGameFinishedDelegate(int exitCode);
        public delegate void TestGameFinishedHandler();
        public event TestGameFinishedHandler TestGameFinished;
        public delegate void TestGameStartingHandler();
        public event TestGameStartingHandler TestGameStarting;

        private Control _mainGUIThread;
        private bool _currentlyTesting = false;
        private Tasks _tasks;

        public InteractiveTasks(Tasks tasks)
        {
            _tasks = tasks;
            _tasks.TestGameFinished += new Tasks.TestGameFinishedHandler(Tasks_TestGameFinished);
        }

        public bool BrowseForAndLoadGame()
        {
            bool loadedSuccessfully = false;
            string gameToLoad = Factory.GUIController.ShowOpenFileDialog("Select game to open", "AGS game files (*.agf, ac2game.dta)|*.agf;ac2game.dta|AGS 3.x games (*.agf)|*.agf|AGS 2.72 games (*.dta)|ac2game.dta", false);
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
                if (((game.SavedXmlVersion != null) &&
                     (game.SavedXmlVersion != AGSEditor.LATEST_XML_VERSION))
                       ||
                    ((game.SavedXmlVersionIndex != null) &&
                     (game.SavedXmlVersionIndex != AGSEditor.LATEST_XML_VERSION_INDEX)))
                {
                    Factory.GUIController.ShowMessage("This game was last saved with " +
                        ((game.SavedXmlEditorVersion == null) ? "an older version" : ("version " + game.SavedXmlEditorVersion))
                        + " of AGS. If you save it now, the game will be upgraded and previous versions of AGS will be unable to open it.", MessageBoxIcon.Information);
                }

                return success;
            }
            catch (Exception ex)
            {
                string messageDetails = string.Empty;
                if ((!(ex is AGS.Types.InvalidDataException)) &&
                    (!(ex is AGS.Types.AGSEditorException)))
                {
                    messageDetails = "\r\n\r\nError details: " + ex.ToString();
                }
                Factory.GUIController.ShowMessage("An error occurred whilst trying to load your game. The error was: " + Environment.NewLine + Environment.NewLine + ex.Message + "\r\n\r\nIf you cannot resolve the error, please post on the AGS Technical Forum for assistance." + messageDetails, MessageBoxIcon.Warning);
                return false;
            }
        }

        public void CreateTemplateFromCurrentGame(string templateFileName)
        {
            BusyDialog.Show("Please wait while the template is created...", new BusyDialog.ProcessingHandler(CreateTemplateFromCurrentGameProcess), templateFileName);
        }

        private object CreateTemplateFromCurrentGameProcess(object templateFileName)
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
            else if (exitCode == ENGINE_EXIT_CODE_CRASH)
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
