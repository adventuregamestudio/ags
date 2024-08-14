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
    class FileCommandsComponent : BaseComponent
    {
        private const string OPEN_GAME_COMMAND = "OpenGame";
        private const string OPEN_RECENT_GAME_COMMAND = "OpenRecentGame";
        private const string SAVE_GAME_COMMAND = "SaveGame";
        private const string GAME_STATS_COMMAND = "GameStatistics";
        private const string JUMP_TO_EVENTS_TAB_COMMAND = "JumpToEventsTab";
        private const string MAKE_TEMPLATE_COMMAND = "CreateTemplate";
        private const string AUTO_NUMBER_SPEECH_COMMAND = "AutoNumberSpeech";
		private const string CREATE_VOICE_ACTING_SCRIPT_COMMAND = "CreateVoiceActingScript";
        private const string EXPORT_GLOBAL_MESSAGES_TO_SCRIPT_COMMAND = "ExportGlobalMessagesToScript";
        private const string REMOVE_GLOBAL_MESSAGES_COMMAND = "RemoveGlobalMessages";
        private const string RECREATE_SPRITEFILE_COMMAND = "RecreateSpriteFile";
        private const string SHOW_PREFERENCES_COMMAND = "ShowPreferences";
        private const string EXIT_COMMAND = "Exit";
        private const string OPEN_PROJ_FILE_EXPLORER_COMMAND = "OpenProjFileExplorerGame";
        private const string DEFAULT_FONT_RESOURCE_PATH = "AGS.Editor.Resources.";

        private List<MenuCommand> _toolbarCommands = new List<MenuCommand>();

        public FileCommandsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _guiController.InteractiveTasks.TestGameStarting += new InteractiveTasks.TestGameStartingHandler(AGSEditor_TestGameStarting);
            _guiController.InteractiveTasks.TestGameFinished += new InteractiveTasks.TestGameFinishedHandler(AGSEditor_TestGameFinished);

            _guiController.RegisterIcon("OpenIcon", Resources.ResourceManager.GetIcon("open.ico"));
            _guiController.RegisterIcon("SaveIcon", Resources.ResourceManager.GetIcon("save.ico"));

            _guiController.RegisterIcon("MenuIconAutoNumber", Resources.ResourceManager.GetIcon("menu_file_auto-number.ico"));
            _guiController.RegisterIcon("MenuIconExit", Resources.ResourceManager.GetIcon("menu_file_exit.ico"));
            _guiController.RegisterIcon("MenuIconMakeTemplate", Resources.ResourceManager.GetIcon("menu_file_make-template.ico"));
            _guiController.RegisterIcon("MenuIconOpen", Resources.ResourceManager.GetIcon("menu_file_open.ico"));
            _guiController.RegisterIcon("MenuIconPreferences", Resources.ResourceManager.GetIcon("menu_file_preferences.ico"));
            _guiController.RegisterIcon("MenuIconSave", Resources.ResourceManager.GetIcon("menu_file_save.ico"));
            _guiController.RegisterIcon("MenuIconStatistics", Resources.ResourceManager.GetIcon("menu_file_stats.ico"));
            _guiController.RegisterIcon("MenuIconVoiceActingScript", Resources.ResourceManager.GetIcon("menu_file_voicescript.ico"));
            _guiController.RegisterIcon("MenuIconGoToEventsGrid", Resources.ResourceManager.GetIcon("menu_file_eventsgrid.ico"));
            _guiController.RegisterIcon("OpenProjFolderIcon", Resources.ResourceManager.GetIcon("menu_file_openfolder.ico"));

            MenuCommands commands = new MenuCommands(GUIController.FILE_MENU_ID, 0);
            commands.Commands.Add(new MenuCommand(OPEN_GAME_COMMAND, "&Open...", Keys.Control | Keys.L, "MenuIconOpen"));
            commands.Commands.Add(GetOpenRecentMenuCommand());
            commands.Commands.Add(new MenuCommand(SAVE_GAME_COMMAND, "&Save", Keys.Control | Keys.S, "MenuIconSave"));
            commands.Commands.Add(new MenuCommand(GAME_STATS_COMMAND, "&Game statistics", Keys.F2, "MenuIconStatistics"));
            commands.Commands.Add(new MenuCommand(JUMP_TO_EVENTS_TAB_COMMAND, "&Go to Events grid", Keys.F4, "MenuIconGoToEventsGrid"));
            _guiController.AddMenuItems(this, commands);

			commands = new MenuCommands(GUIController.FILE_MENU_ID, 100);
			commands.Commands.Add(new MenuCommand(MAKE_TEMPLATE_COMMAND, "&Make template from this game...", "MenuIconMakeTemplate"));
            commands.Commands.Add(new MenuCommand(AUTO_NUMBER_SPEECH_COMMAND, "&Auto-number speech lines...", "MenuIconAutoNumber"));
			commands.Commands.Add(new MenuCommand(CREATE_VOICE_ACTING_SCRIPT_COMMAND, "Create &voice acting script...", "MenuIconVoiceActingScript"));
            commands.Commands.Add(new MenuCommand(RECREATE_SPRITEFILE_COMMAND, "Restore all sprites from sources"));
            _guiController.AddMenuItems(this, commands);

            commands = new MenuCommands(GUIController.FILE_MENU_ID, 110);
            var subCommands = new List<MenuCommand>
            {
                new MenuCommand(EXPORT_GLOBAL_MESSAGES_TO_SCRIPT_COMMAND, "Export Global Messages to script"),
                new MenuCommand(REMOVE_GLOBAL_MESSAGES_COMMAND, "Remove Global Messages")
            };
            commands.Commands.Add(new MenuCommand(null, "Global Messages", Keys.None, null, subCommands));
            _guiController.AddMenuItems(this, commands);

            commands = new MenuCommands(GUIController.FILE_MENU_ID, 800);
            commands.Commands.Add(new MenuCommand(OPEN_PROJ_FILE_EXPLORER_COMMAND, "Open Project in File Explorer", "OpenProjFolderIcon"));
            _guiController.AddMenuItems(this, commands);

            commands = new MenuCommands(GUIController.FILE_MENU_ID, 9000);
            commands.Commands.Add(new MenuCommand(SHOW_PREFERENCES_COMMAND, "&Preferences...", "MenuIconPreferences"));
            _guiController.AddMenuItems(this, commands);

            commands = new MenuCommands(GUIController.FILE_MENU_ID, 9999);
			commands.Commands.Add(new MenuCommand(EXIT_COMMAND, "E&xit", Keys.Control | Keys.Q, "MenuIconExit"));
			_guiController.AddMenuItems(this, commands);

            MenuCommand openIcon = new MenuCommand(OPEN_GAME_COMMAND, "Open game (Ctrl+L)", "OpenIcon");
            MenuCommand saveIcon = new MenuCommand(SAVE_GAME_COMMAND, "Save game (Ctrl+S)", "SaveIcon");
            _toolbarCommands.Add(openIcon);
            _toolbarCommands.Add(saveIcon);
            Factory.ToolBarManager.AddGlobalItems(this, _toolbarCommands);

            Factory.AGSEditor.Settings.AfterRecentGamesChanged += FileCommandsComponent_AfterRecentGamesChanged;
        }

        private void FileCommandsComponent_AfterRecentGamesChanged(object sender)
        {
            RefreshRecentGamesMenu();
        }

        private string GetRecentGameText(string name, string path)
        {
            return name + "     (" + path + ")";
        }

        private string GetRecentGameMenuText(Preferences.RecentGame game, int maxPrintSize)
        {
            int maxPrintPathSize = maxPrintSize - GetRecentGameText(game.Name, string.Empty).Length;
            string printablePath = game.Path.Length > maxPrintPathSize
                ? "..." + game.Path.Substring(game.Path.Length - maxPrintPathSize)
                : game.Path;

            return GetRecentGameText(game.Name, printablePath);
        }

        private List<MenuCommand> GetRecentGamesSubcommands()
        {
            const int maxPrintSize = 80; // Maximum char length for recent games text
            var subCommands = new List<MenuCommand>();
            
            if(Factory.AGSEditor.Settings.RecentGames.Count <= 1)
            {
                MenuCommand cmd = new MenuCommand(null, "(empty)");
                cmd.Enabled = false;
                subCommands.Add(cmd);
                return subCommands;
            }

            // we start from index 1 because 0 is always currently open game
            for (int i=1; i< Factory.AGSEditor.Settings.RecentGames.Count; i++)  
            {
                var game = Factory.AGSEditor.Settings.RecentGames[i];
                if (string.IsNullOrEmpty(game.Path) || !Directory.Exists(game.Path))
                    continue;

                string projectPath = Path.Combine(game.Path, AGSEditor.GAME_FILE_NAME);
                if (File.Exists(projectPath))
                {
                    string cmdText = GetRecentGameMenuText(game, maxPrintSize);
                    MenuCommand cmd = new MenuCommand(OPEN_RECENT_GAME_COMMAND + i.ToString(), cmdText);
                    subCommands.Add(cmd);
                }
            }
            return subCommands;
        }

        private MenuCommand GetOpenRecentMenuCommand()
        {
            return new MenuCommand(OPEN_RECENT_GAME_COMMAND, "Open Recent", Keys.None, null, GetRecentGamesSubcommands());
        }

        private void RefreshRecentGamesMenu()
        {
            _guiController.ReplaceMenuSubCommands(this, OPEN_RECENT_GAME_COMMAND, GetRecentGamesSubcommands());
        }

        public override string ComponentID
        {
            get { return ComponentIDs.FileCommands; }
        }

        private void LoadGame(string projectPath = null)
        {
            if (_guiController.QueryWhetherToSaveGameBeforeContinuing("Do you want to save the current game before loading a different one?"))
            {
                if(string.IsNullOrEmpty(projectPath))
                {
                    _guiController.InteractiveTasks.BrowseForAndLoadGame();
                }
                else
                {
                    _guiController.InteractiveTasks.LoadGame(projectPath);
                }
            }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == OPEN_GAME_COMMAND)
            {
                LoadGame();
            }
            else if (controlID == SAVE_GAME_COMMAND)
            {
                _agsEditor.SaveGameFiles();
            }
            else if (controlID == GAME_STATS_COMMAND)
            {
                Game game = _agsEditor.CurrentGame;
                StringBuilder sb = new StringBuilder(1000);
                int numSprites = CountSprites(game.RootSpriteFolder);
                sb.AppendFormat("Total sprites:\t{0} / {1}\n", numSprites, NativeConstants.MAX_STATIC_SPRITES);
                int numSpriteFolders = CountSpriteFolders(game.RootSpriteFolder);
                sb.AppendFormat("Sprite folders:\t{0}\n", numSpriteFolders);

                sb.AppendFormat("Total views:\t{0}\n", game.ViewCount);
                sb.AppendFormat("Total GUIs:\t{0}\n", game.RootGUIFolder.GetAllItemsCount());
                sb.AppendFormat("Inventory Items:\t{0} / {1}\n", game.RootInventoryItemFolder.GetAllItemsCount(), Game.MAX_INV_ITEMS);
                sb.AppendFormat("Characters:\t{0}\n", game.RootCharacterFolder.GetAllItemsCount());
                sb.AppendFormat("Dialog topics:\t{0}\n", game.RootDialogFolder.GetAllItemsCount());

                _guiController.ShowMessage(sb.ToString(), MessageBoxIcon.Information);
            }
            else if (controlID == JUMP_TO_EVENTS_TAB_COMMAND)
            {
                if (_guiController.SelectEventsTabInPropertyGrid())
                {
                    _guiController.MoveMouseCursorToPropertyGrid();
                }
                else
                {
                    _guiController.ShowMessage("There is no Events grid for the current window. Use this command to jump straight to the Events grid when you are editing something that has interactions, such as a Character, Hotspot or Object.", MessageBoxIcon.Information);
                }
            }
            else if (controlID == MAKE_TEMPLATE_COMMAND)
            {
                _guiController.SaveGameAsTemplate();
            }
            else if (controlID == AUTO_NUMBER_SPEECH_COMMAND)
            {
                bool proceed = true;
                if (_agsEditor.CurrentGame.Translations.Count > 0)
                {
                    proceed = _guiController.ShowQuestion("Your game has one or more translations. If you proceed with re-numbering speech lines, this could break your translations of those lines. Are you sure you want to continue?", MessageBoxIcon.Warning) == DialogResult.Yes;
                }
                if (proceed)
                {
                    _guiController.ShowAutoNumberSpeechWizard();
                }
            }
            else if (controlID == CREATE_VOICE_ACTING_SCRIPT_COMMAND)
            {
                _guiController.ShowCreateVoiceActingScriptWizard();
            }
            else if (controlID == EXPORT_GLOBAL_MESSAGES_TO_SCRIPT_COMMAND)
            {
                if (_guiController.ShowQuestion("This will move any remaining AGS 2.x Global Messages from this game's internal data to the generated script module. This will let you see these texts again and do with them whatever you prefer. But this will also make script commands like DisplayMessage no longer work, and you will have to replace them with contemporary commands for displaying these texts. Are you sure you want to do this?") == DialogResult.Yes)
                {
                    ExportGlobalMessagesToScript();
                }
            }
            else if (controlID == REMOVE_GLOBAL_MESSAGES_COMMAND)
            {
                if (_guiController.ShowQuestion("This will remove all traces of AGS 2.x Global Messages from this game. Do not proceed if you are still using any of the Global Messages that you created with a previous version of AGS. Are you sure you want to do this?") == DialogResult.Yes)
                {
                    RemoveGlobalMessagesFromGame();
                }
            }
            else if (controlID == RECREATE_SPRITEFILE_COMMAND)
            {
                if (_guiController.ShowQuestion("This will recreate game's spritefile using sprite source files if they are available. All sprites will be updated from their sources.\n\nNOTE: sprites that don't have source file references, or which source files are missing, - will remain untouched.\n\nAre you sure you want to do this?",
                    MessageBoxIcon.Warning) == DialogResult.Yes)
                {
                    Tasks.RecreateSpriteFileFromSources();
                }
            }
            else if (controlID == SHOW_PREFERENCES_COMMAND)
            {
                _guiController.ShowPreferencesEditor();
            }
            else if(controlID == OPEN_PROJ_FILE_EXPLORER_COMMAND)
            {
                Utilities.OpenFileOrDirInFileExplorer(Path.Combine(Factory.AGSEditor.CurrentGame.DirectoryPath, AGSEditor.GAME_FILE_NAME));
            }
            else if (controlID == EXIT_COMMAND)
            {
                _guiController.ExitApplication();
            }
            else if (controlID.StartsWith(OPEN_RECENT_GAME_COMMAND) && controlID.Length > OPEN_RECENT_GAME_COMMAND.Length)
            {
                int index = int.Parse(controlID.Split(new string[] { OPEN_RECENT_GAME_COMMAND }, StringSplitOptions.None)[1]);
                var game = Factory.AGSEditor.Settings.RecentGames[index];
                string projectPath = Path.Combine(game.Path, AGSEditor.GAME_FILE_NAME);                
                LoadGame(projectPath);
            }
        }

        public override void RefreshDataFromGame()
        {
            bool globalMessagesExist = false;
            foreach (string globalMessage in _agsEditor.CurrentGame.GlobalMessages)
            {
                if (!string.IsNullOrEmpty(globalMessage))
                {
                    globalMessagesExist = true;
                    break;
                }
            }

            _guiController.SetMenuItemEnabled(this, EXPORT_GLOBAL_MESSAGES_TO_SCRIPT_COMMAND, globalMessagesExist);
            _guiController.SetMenuItemEnabled(this, REMOVE_GLOBAL_MESSAGES_COMMAND, globalMessagesExist);
        }

        private void ExportGlobalMessagesToScript()
        {
            int validMessages = 0;
            for (int i = 0; i < _agsEditor.CurrentGame.GlobalMessages.Length; i++)
            {
                if (!string.IsNullOrEmpty(_agsEditor.CurrentGame.GlobalMessages[i]))
                    validMessages++;
            }

            if (validMessages > 0)
            {
                string scriptName = _agsEditor.Tasks.GenerateScriptWithGlobalMessages("_GlobalMessages");
                if (scriptName == null)
                {
                    _guiController.ShowMessage("Global Messages were not moved, operation cancelled", MessageBoxIcon.Exclamation);
                    return;
                }

                for (int i = 0; i < _agsEditor.CurrentGame.GlobalMessages.Length; i++)
                {
                    _agsEditor.CurrentGame.GlobalMessages[i] = string.Empty;
                }

                _guiController.ShowMessage(
                    string.Format("{0} Global Messages were moved to a String array in {1}.",
                        validMessages, scriptName),
                    MessageBoxIcon.Information);
            }
            else
            {
                _guiController.ShowMessage("No valid Global Messages were found, nothing to move.", MessageBoxIcon.Information);
            }
            RefreshDataFromGame(); // update menu state
        }

        private void RemoveGlobalMessagesFromGame()
        {
            int messagesRemoved = 0;
            for (int i = 0; i < _agsEditor.CurrentGame.GlobalMessages.Length; i++)
            {
                if (!string.IsNullOrEmpty(_agsEditor.CurrentGame.GlobalMessages[i]))
                {
                    _agsEditor.CurrentGame.GlobalMessages[i] = string.Empty;
                    messagesRemoved++;
                }
            }

            _guiController.ShowMessage(messagesRemoved.ToString() + " Global Messages were removed.", MessageBoxIcon.Information);
            RefreshDataFromGame(); // update menu state
        }

        private int CountSprites(SpriteFolder folder)
        {
            int total = folder.Sprites.Count;

            foreach (SpriteFolder subFolder in folder.SubFolders)
            {
                total += CountSprites(subFolder);
            }

            return total;
        }

        private int CountSpriteFolders(SpriteFolder folder)
        {
            int total = folder.SubFolders.Count;

            foreach (SpriteFolder subFolder in folder.SubFolders)
            {
                total += CountSpriteFolders(subFolder);
            }

            return total;
        }

        private void SetMenuOptionsEnabledStateForTestingGame(bool enabled)
        {
            _guiController.SetMenuItemEnabled(this, OPEN_GAME_COMMAND, enabled);
            _guiController.SetMenuItemEnabled(this, SAVE_GAME_COMMAND, enabled);
            _guiController.SetMenuItemEnabled(this, EXIT_COMMAND, enabled);
            foreach (MenuCommand command in _toolbarCommands)
            {
                if ((command.ID == OPEN_GAME_COMMAND) ||
                    (command.ID == SAVE_GAME_COMMAND))
                {
                    command.Enabled = enabled;
                }
            }
            Factory.ToolBarManager.UpdateItemEnabledStates(_toolbarCommands);
        }

        private void AGSEditor_TestGameFinished()
        {
            SetMenuOptionsEnabledStateForTestingGame(true);
            _guiController.SetTitleBarPrefix(string.Empty);
        }

        private void AGSEditor_TestGameStarting()
        {
            SetMenuOptionsEnabledStateForTestingGame(false);
            _guiController.SetTitleBarPrefix("[Running] ");
        }

        public override void EditorShutdown()
        {
            Factory.AGSEditor.Settings.AfterRecentGamesChanged -= FileCommandsComponent_AfterRecentGamesChanged;
        }
    }
}
