using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Windows.Forms;
using System.Xml;

namespace AGS.Editor.Components
{
    class BuildCommandsComponent : BaseComponent
    {
        private const string DEBUG_MENU_ID = "DebugMenuHeader";

        private const string COMPILE_GAME_COMMAND = "CompileGame";
		private const string REBUILD_GAME_COMMAND = "RebuildGame";
        private const string REBUILD_ALL_PLATFORMS_COMMAND = "RebuildAllPlatforms";
		private const string SETUP_GAME_COMMAND = "SetupGame";
        private const string TEST_GAME_COMMAND = "TestGame";
        private const string RUN_COMMAND = "RunGame";
        private const string STEP_INTO_COMMAND = "StepIntoDebug";
        private const string STOP_COMMAND = "StopDebug";
        private const string PAUSE_COMMAND = "PauseDebug";

        private List<MenuCommand> _debugToolbarCommands = new List<MenuCommand>();
        private DebugState _debuggerState = DebugState.NotRunning;
        private bool _testGameInProgress = false;

        public BuildCommandsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            ScriptEditor.AttemptToEditScript += new ScriptEditor.AttemptToEditScriptHandler(ScriptEditor_AttemptToEditScript);
            _guiController.QueryEditorShutdown += new GUIController.QueryEditorShutdownHandler(guiController_QueryEditorShutdown);
            _guiController.InteractiveTasks.TestGameStarting += new InteractiveTasks.TestGameStartingHandler(AGSEditor_TestGameStarting);
            _guiController.InteractiveTasks.TestGameFinished += new InteractiveTasks.TestGameFinishedHandler(AGSEditor_TestGameFinished);
            _guiController.RegisterIcon("BuildIcon", Resources.ResourceManager.GetIcon("build.ico"));
            _guiController.RegisterIcon("RunIcon", Resources.ResourceManager.GetIcon("run.ico"));
            _guiController.RegisterIcon("StepIcon", Resources.ResourceManager.GetIcon("step.ico"));
            _guiController.RegisterIcon("StopIcon", Resources.ResourceManager.GetIcon("stop.ico"));
            _guiController.RegisterIcon("PauseIcon", Resources.ResourceManager.GetIcon("pause.ico"));
            _guiController.RegisterIcon("RunMenuIcon", Resources.ResourceManager.GetIcon("menu_build_run.ico"));
            _guiController.RegisterIcon("StepMenuIcon", Resources.ResourceManager.GetIcon("menu_build_step-into.ico"));
            _guiController.RegisterIcon("StopMenuIcon", Resources.ResourceManager.GetIcon("menu_build_stop.ico"));
            _guiController.RegisterIcon("PauseMenuIcon", Resources.ResourceManager.GetIcon("menu_build_pause.ico"));
			_guiController.RegisterIcon("RebuildAllMenuIcon", Resources.ResourceManager.GetIcon("menu_build_rebuild-files.ico"));
            _guiController.RegisterIcon("RebuildAllPlatformsMenuIcon", Resources.ResourceManager.GetIcon("menu_build_rebuild-files.ico"));
			_guiController.RegisterIcon("SetupGameMenuIcon", Resources.ResourceManager.GetIcon("menu_build_gamesetup.ico"));

            _guiController.RegisterIcon("MenuIconBuildEXE", Resources.ResourceManager.GetIcon("menu_file_built-exe.ico"));
            _guiController.RegisterIcon("MenuIconTest", Resources.ResourceManager.GetIcon("menu_build_runwithout.ico"));

            _guiController.AddMenu(this, DEBUG_MENU_ID, "&Build");
            MenuCommands debugCommands = new MenuCommands(DEBUG_MENU_ID, GUIController.FILE_MENU_ID);
            debugCommands.Commands.Add(new MenuCommand(RUN_COMMAND, "&Run", Keys.F5, "RunMenuIcon"));
            debugCommands.Commands.Add(new MenuCommand(TEST_GAME_COMMAND, "Run without &debugger", Keys.Control | Keys.F5, "MenuIconTest"));
            debugCommands.Commands.Add(new MenuCommand(STEP_INTO_COMMAND, "S&tep into", Keys.F11, "StepMenuIcon"));
            debugCommands.Commands.Add(new MenuCommand(PAUSE_COMMAND, "&Pause", "PauseMenuIcon"));
            debugCommands.Commands.Add(new MenuCommand(STOP_COMMAND, "&Stop", "StopMenuIcon"));
            debugCommands.Commands.Add(MenuCommand.Separator);
            debugCommands.Commands.Add(new MenuCommand(COMPILE_GAME_COMMAND, "&Build EXE", Keys.F7, "MenuIconBuildEXE"));
			debugCommands.Commands.Add(new MenuCommand(REBUILD_GAME_COMMAND, "Rebuild &all files", "RebuildAllMenuIcon"));
            debugCommands.Commands.Add(new MenuCommand(REBUILD_ALL_PLATFORMS_COMMAND, "Re&build all platforms", "RebuildAllPlatformsMenuIcon"));
			debugCommands.Commands.Add(new MenuCommand(SETUP_GAME_COMMAND, "Run game setu&p...", "SetupGameMenuIcon"));
            _guiController.AddMenuItems(this, debugCommands);

            _guiController.SetMenuItemEnabled(this, STEP_INTO_COMMAND, false);
            _guiController.SetMenuItemEnabled(this, PAUSE_COMMAND, false);
            _guiController.SetMenuItemEnabled(this, STOP_COMMAND, false);

            MenuCommand buildIcon = new MenuCommand(COMPILE_GAME_COMMAND, "Build game EXE (F7)", "BuildIcon");
            MenuCommand runIcon = new MenuCommand(RUN_COMMAND, "Run (F5)", "RunIcon");
            MenuCommand stopIcon = new MenuCommand(STOP_COMMAND, "Stop", "StopIcon");
            MenuCommand stepIcon = new MenuCommand(STEP_INTO_COMMAND, "Step into (F11)", "StepIcon");
            MenuCommand pauseIcon = new MenuCommand(PAUSE_COMMAND, "Pause", "PauseIcon");
            stepIcon.Enabled = false;
            stopIcon.Enabled = false;
            pauseIcon.Enabled = false;
            _debugToolbarCommands.Add(buildIcon);
            _debugToolbarCommands.Add(runIcon);
            _debugToolbarCommands.Add(pauseIcon);
            _debugToolbarCommands.Add(stepIcon);
            _debugToolbarCommands.Add(stopIcon);
            Factory.ToolBarManager.AddGlobalItems(this, _debugToolbarCommands);
            Factory.ToolBarManager.UpdateItemEnabledStates(_debugToolbarCommands);

            _agsEditor.Debugger.DebugStateChanged += new DebugController.DebugStateChangedHandler(Debugger_DebugStateChanged);
            _agsEditor.AttemptToSaveGame += new AGSEditor.AttemptToSaveGameHandler(_agsEditor_AttemptToSaveGame);
        }

        private void ScriptEditor_AttemptToEditScript(ref bool allowEdit)
        {
            if (_testGameInProgress)
            {
                _guiController.ShowMessage("You cannot edit scripts while the game is being tested. Close the game first.", MessageBoxIcon.Warning);
                allowEdit = false;
            }
        }

        private void _agsEditor_AttemptToSaveGame(ref bool allowSave)
        {
            if (_testGameInProgress)
            {
                _guiController.ShowMessage("You cannot save your changes while the game is being tested. Close the game first.", MessageBoxIcon.Warning);
                allowSave = false;
            }
        }

        private void Debugger_DebugStateChanged(DebugState newState)
        {
            if (_guiController.InvokeRequired)
            {
                _guiController.Invoke(new DebugController.DebugStateChangedHandler(Debugger_DebugStateChanged), newState);
                return;
            }

            _debuggerState = newState;

            if (newState == DebugState.Running)
            {
                _guiController.SetTitleBarPrefix("[Debugging] ");
            }
            else if (newState == DebugState.Paused)
            {
                _guiController.SetTitleBarPrefix("[Break] ");
            }
            else
            {
                _guiController.SetTitleBarPrefix(string.Empty);
                _guiController.HideCallStack();
                _guiController.HideFindSymbolResults();
            }

            foreach (MenuCommand command in _debugToolbarCommands)
            {
                if (command.ID == STOP_COMMAND)
                {
                    command.Enabled = (newState != DebugState.NotRunning);
                    _guiController.SetMenuItemEnabled(this, STOP_COMMAND, command.Enabled);
                }
                else if (command.ID == PAUSE_COMMAND)
                {
                    command.Enabled = (newState == DebugState.Running);
                    _guiController.SetMenuItemEnabled(this, PAUSE_COMMAND, command.Enabled);
                }
                else if (command.ID == STEP_INTO_COMMAND)
                {
                    command.Enabled = (newState == DebugState.Paused);
                    _guiController.SetMenuItemEnabled(this, command.ID, command.Enabled);
                }
                else if (command.ID == RUN_COMMAND)
                {
                    command.Enabled = (newState != DebugState.Running);
                    _guiController.SetMenuItemEnabled(this, command.ID, command.Enabled);
                }
            }
            Factory.ToolBarManager.UpdateItemEnabledStates(_debugToolbarCommands);
        }

        public override string ComponentID
        {
            get { return ComponentIDs.BuildCommands; }
        }

        private void TestGame(bool withDebugger)
        {
            bool forceRebuild;

            forceRebuild = _agsEditor.NeedsRebuildForDebugMode();
            if (_agsEditor.SaveGameFiles())
            {
                if (!_agsEditor.CompileGame(forceRebuild, true).HasErrors)
                {
                    _testGameInProgress = true;
                    _guiController.InteractiveTasks.TestGame(withDebugger);
                }
            }
        }

        public override void CommandClick(string controlID)
        {
			if ((controlID == RUN_COMMAND) &&
				(!_agsEditor.Debugger.CanUseDebugger))
			{
				controlID = TEST_GAME_COMMAND;
			}

            if (controlID == RUN_COMMAND)
            {
				_guiController.ShowCuppit("When using this Run command, the game will always run in a window. If you want to play full-screen, use the Run Without Debugger option.", "Test full screen warning", true);
				
                if (_debuggerState == DebugState.NotRunning)
                {
                    TestGame(true);
                }
                else
                {
                    _agsEditor.Debugger.Resume();
                }
            }
            else if (controlID == STEP_INTO_COMMAND)
            {
                _agsEditor.Debugger.StepInto();
            }
            else if (controlID == STOP_COMMAND)
            {
                _agsEditor.Debugger.StopDebugging();
            }
            else if (controlID == PAUSE_COMMAND)
            {
                _agsEditor.Debugger.PauseExecution();
            }
            else if (controlID == COMPILE_GAME_COMMAND)
            {
				CompileGame(false);
			}
			else if (controlID == REBUILD_GAME_COMMAND)
			{
				CompileGame(true);
			}
            else if (controlID == REBUILD_ALL_PLATFORMS_COMMAND)
            {
                CompileGameForAllPlatforms(true);
            }
            else if (controlID == SETUP_GAME_COMMAND)
            {
                try
                {
                    _agsEditor.Tasks.RunGameSetup();
                }
                catch (FileNotFoundException ex)
                {
                    _guiController.ShowMessage(ex.Message, MessageBoxIcon.Warning);
                }
            }
            else if (controlID == TEST_GAME_COMMAND)
            {
                TestGame(false);
            }
        }

		private void CompileGame(bool forceRebuild)
		{
			forceRebuild = _agsEditor.NeedsRebuildForDebugMode() || forceRebuild;
			if (_agsEditor.SaveGameFiles())
			{
				if (_agsEditor.CompileGame(forceRebuild, false).Count == 0)
				{
					_guiController.ShowMessage("Compile successful!", MessageBoxIcon.Information);
				}
			}
		}

        private void CompileGameForAllPlatforms(bool forceRebuild)
        {
            forceRebuild = _agsEditor.NeedsRebuildForDebugMode() || forceRebuild;
            if (_agsEditor.SaveGameFiles())
            {
                if (_agsEditor.CompileGame(forceRebuild, false).Count == 0)
                {
                    Factory.Events.OnBuildAllPlatforms();
                    _guiController.ShowMessage("Compile successful!", MessageBoxIcon.Information);
                }
            }
        }

		private bool guiController_QueryEditorShutdown()
        {
            if (_testGameInProgress)
            {
                _guiController.ShowMessage("You can't close the editor while testing your game. Quit the game first.", MessageBoxIcon.Warning);
                return false;
            }
            return true;
        }

        private void AGSEditor_TestGameStarting()
        {
            _guiController.SetMenuItemEnabled(this, COMPILE_GAME_COMMAND, false);
			_guiController.SetMenuItemEnabled(this, REBUILD_GAME_COMMAND, false);
			_guiController.SetMenuItemEnabled(this, TEST_GAME_COMMAND, false);
            _guiController.SetMenuItemEnabled(this, RUN_COMMAND, false);
            foreach (MenuCommand command in _debugToolbarCommands)
            {
                if ((command.ID == COMPILE_GAME_COMMAND) ||
                    (command.ID == RUN_COMMAND))
                {
                    command.Enabled = false;
                }
            }
            Factory.ToolBarManager.UpdateItemEnabledStates(_debugToolbarCommands);

            _testGameInProgress = true;
        }

        private void AGSEditor_TestGameFinished()
        {
            _guiController.SetMenuItemEnabled(this, COMPILE_GAME_COMMAND, true);
			_guiController.SetMenuItemEnabled(this, REBUILD_GAME_COMMAND, true);
			_guiController.SetMenuItemEnabled(this, TEST_GAME_COMMAND, true);
            _guiController.SetMenuItemEnabled(this, RUN_COMMAND, true);
            foreach (MenuCommand command in _debugToolbarCommands)
            {
                if ((command.ID == COMPILE_GAME_COMMAND) ||
                    (command.ID == RUN_COMMAND))
                {
                    command.Enabled = true;
                }
            }
            Factory.ToolBarManager.UpdateItemEnabledStates(_debugToolbarCommands);

            _testGameInProgress = false;
        }

        public override void RefreshDataFromGame()
        {
            base.RefreshDataFromGame();
            Factory.AGSEditor.RefreshBuildPlatforms(Factory.AGSEditor.CurrentGame.Settings.TargetPlatforms);
        }
    }
}
