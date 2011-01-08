using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class RoomsComponent : BaseComponentWithScripts, IRoomController
    {
        private const string TOP_LEVEL_COMMAND_ID = "Rooms";
        private const string COMMAND_NEW_ITEM = "NewRoom";
        private const string COMMAND_ADD_EXISTING_ROOM = "AddExistingRoom";
		private const string COMMAND_SORT_BY_NUMBER = "SortByNumber";
        private const string COMMAND_DELETE_ITEM = "DeleteRoom";
        private const string COMMAND_EXCLUDE_ITEM = "ExcludeRoom";
		private const string COMMAND_CREATE_TEMPLATE = "TemplateFromRoom";
        private const string TREE_PREFIX_ROOM_NODE = "Rom";
        private const string TREE_PREFIX_ROOM_SETTINGS = "Roe";
        private const string TREE_PREFIX_ROOM_SCRIPT = "Ros";
        private const string COMMAND_LOAD_ROOM = "LoadRoom";
        private const string COMMAND_SAVE_ROOM = "SaveRoom";

        private const string ROOM_ICON_UNLOADED = "RoomIcon";
        private const string ROOM_ICON_LOADED = "RoomColourIcon";

        public event PreSaveRoomHandler PreSaveRoom;
        private ContentDocument _roomSettings;
        private Dictionary<int,ContentDocument> _roomScriptEditors = new Dictionary<int,ContentDocument>();
        private Room _loadedRoom;
		private NativeProxy _nativeProxy;
        private int _rightClickedRoomNumber;
        private Room.RoomModifiedChangedHandler _modifiedChangedHandler;
		private object _roomLoadingOrSavingLock = new object();

        public RoomsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _guiController.RegisterIcon("RoomsIcon", Resources.ResourceManager.GetIcon("room.ico"));
            _guiController.RegisterIcon(ROOM_ICON_UNLOADED, Resources.ResourceManager.GetIcon("room-item.ico"));
            _guiController.RegisterIcon(ROOM_ICON_LOADED, Resources.ResourceManager.GetIcon("roomsareas.ico"));
            _guiController.RegisterIcon("MenuIconSaveRoom", Resources.ResourceManager.GetIcon("menu_file_save-room.ico"));

            MenuCommands commands = new MenuCommands(GUIController.FILE_MENU_ID, 50);
            commands.Commands.Add(new MenuCommand(COMMAND_SAVE_ROOM, "Save Room", Keys.Control | Keys.R, "MenuIconSaveRoom"));
            _guiController.AddMenuItems(this, commands);

			_nativeProxy = Factory.NativeProxy;
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Rooms", "RoomsIcon");
            _guiController.OnZoomToFile += new GUIController.ZoomToFileHandler(GUIController_OnZoomToFile);
            _guiController.OnGetScript += new GUIController.GetScriptHandler(GUIController_OnGetScript);
            _guiController.OnScriptChanged += new GUIController.ScriptChangedHandler(GUIController_OnScriptChanged);
            _guiController.OnGetScriptEditorControl += new GUIController.GetScriptEditorControlHandler(_guiController_OnGetScriptEditorControl);
            _agsEditor.PreCompileGame += new AGSEditor.PreCompileGameHandler(AGSEditor_PreCompileGame);
            _agsEditor.PreSaveGame += new AGSEditor.PreSaveGameHandler(AGSEditor_PreSaveGame);
            _agsEditor.ProcessAllGameTexts += new AGSEditor.ProcessAllGameTextsHandler(AGSEditor_ProcessAllGameTexts);
			_agsEditor.PreDeleteSprite += new AGSEditor.PreDeleteSpriteHandler(AGSEditor_PreDeleteSprite);
            _modifiedChangedHandler = new Room.RoomModifiedChangedHandler(_loadedRoom_RoomModifiedChanged);
            RePopulateTreeView();
        }

        private void _guiController_OnGetScriptEditorControl(GetScriptEditorControlEventArgs evArgs)
        {
            var scriptEditor = GetScriptEditor(evArgs.ScriptFileName, evArgs.ShowEditor);
            if (scriptEditor != null)
            {
                evArgs.ScriptEditor = scriptEditor.ScriptEditorControl;
            }
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Rooms; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
				List<RoomTemplate> templates = ConstructListOfRoomTemplates();
				NewRoomDialog dialog = new NewRoomDialog(_agsEditor.CurrentGame, templates);
				if (dialog.ShowDialog() == DialogResult.OK)
				{
					CreateNewRoom(dialog.ChosenRoomNumber, dialog.ChosenTemplate);
				}
				dialog.Dispose();
				DisposeIcons(templates);
            }
            else if (controlID == COMMAND_ADD_EXISTING_ROOM)
            {
                string fileName = _guiController.ShowOpenFileDialog("Choose room to import...", "AGS room files (*.crm)|*.crm");
                if (fileName != null)
                {
                    ImportExistingRoomFile(fileName);
                }
            }
			else if (controlID == COMMAND_CREATE_TEMPLATE)
			{
				CreateTemplateFromRoom(_rightClickedRoomNumber);
			}
			else if (controlID == COMMAND_EXCLUDE_ITEM)
			{
				if (MessageBox.Show("Are you sure you want to remove this room from the game? The file will not be deleted.", "Confirm exclude", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
				{
                    if (EnsureNoCharactersStartInRoom(_rightClickedRoomNumber))
                    {
                        RemoveRoomFromGame(_rightClickedRoomNumber);
                    }
				}
			}
			else if (controlID == COMMAND_DELETE_ITEM)
			{
				if (MessageBox.Show("Are you sure you want to delete this room? It cannot be recovered.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
				{
                    if (EnsureNoCharactersStartInRoom(_rightClickedRoomNumber))
                    {
                        DeleteRoom(_rightClickedRoomNumber);
                    }
				}
			}
			else if (controlID == COMMAND_SORT_BY_NUMBER)
			{
				RoomList roomList = (RoomList)_agsEditor.CurrentGame.Rooms;
				roomList.SortByNumber();
				RePopulateTreeView();
                RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
            }
			else if (controlID.StartsWith(TREE_PREFIX_ROOM_SETTINGS))
			{
				LoadRoom(controlID);
			}
			else if (controlID.StartsWith(TREE_PREFIX_ROOM_SCRIPT))
			{
				UnloadedRoom selectedRoom = FindRoomByID(Convert.ToInt32(controlID.Substring(3)));
				if (selectedRoom.Script == null)
				{
					selectedRoom.LoadScript();
				}

				CreateOrShowScript(selectedRoom);
			}
			else if (controlID == COMMAND_LOAD_ROOM)
			{
                if ((_loadedRoom == null) || (_roomSettings == null) ||
                    (_loadedRoom.Number != _rightClickedRoomNumber))
                {
                    UnloadedRoom selectedRoom = FindRoomByID(_rightClickedRoomNumber);
                    LoadDifferentRoom(selectedRoom);
                }

				if (_roomSettings != null)
				{
					_guiController.AddOrShowPane(_roomSettings);
				}
			}
			else if (controlID == COMMAND_SAVE_ROOM)
			{
				if (_loadedRoom != null)
				{
					SaveRoomAndShowAnyErrors(_loadedRoom);
				}
			}
			else if (controlID.StartsWith(TREE_PREFIX_ROOM_NODE))
			{
				LoadRoom(controlID);
			}
        }

        private bool EnsureNoCharactersStartInRoom(int roomNumber)
        {
            bool okToContinue = true;
            List<Character> charactersThatStartInThisRoom = new List<Character>();
            string characterList = string.Empty;
            foreach (var character in _agsEditor.CurrentGame.Characters)
            {
                if (character.StartingRoom == roomNumber)
                {
                    charactersThatStartInThisRoom.Add(character);
                    characterList += string.Format("{0} ({1}){2}", character.ID, character.ScriptName, Environment.NewLine);
                }
            }
            if (charactersThatStartInThisRoom.Count > 0)
            {
                if (_guiController.ShowQuestion("The following characters are set to start in this room. If you remove the room, they will be set to have no starting room. Do you want to continue?" + 
                    Environment.NewLine + Environment.NewLine + characterList, MessageBoxIcon.Warning) == DialogResult.No)
                {
                    okToContinue = false;
                }
                else
                {
                    charactersThatStartInThisRoom.ForEach(c => c.StartingRoom = -1);
                }
            }
            return okToContinue;
        }

        private void DeleteRoom(int roomNumber)
        {
            UnloadedRoom roomToDelete = RemoveRoomFromGame(roomNumber);

            CloseRoomScriptEditorIfOpen(roomToDelete.Number);

            List<string> filesToDelete = new List<string>();

            if (File.Exists(roomToDelete.FileName))
            {
                filesToDelete.Add(roomToDelete.FileName);
            }
            else
            {
                _guiController.ShowMessage("The room file was not found and could not be deleted.", MessageBoxIcon.Warning);
            }

            if (File.Exists(roomToDelete.ScriptFileName))
            {
                filesToDelete.Add(roomToDelete.ScriptFileName);
            }

            try
            {
                _agsEditor.DeleteFileOnDiskAndSourceControl(filesToDelete.ToArray());
            }
            catch (CannotDeleteFileException ex)
            {
                _guiController.ShowMessage("The room file could not be deleted." + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
            }
            RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
            _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
        }

		private void CreateTemplateFromRoom(int roomNumber)
		{
			UnloadedRoom roomToTemplatize = FindRoomByID(roomNumber);
			if ((_loadedRoom != null) && (_loadedRoom.Modified))
			{
				if (!SaveRoomAndShowAnyErrors(_loadedRoom))
				{
					return;
				}
			}

			if ((_loadedRoom == null) || (_loadedRoom.Number != roomNumber))
			{
				LoadDifferentRoom(roomToTemplatize);
			}

			bool createTemplate = true;
			
			foreach (RoomObject obj in _loadedRoom.Objects)
			{
				if (obj.Image > 0)
				{
					if (_guiController.ShowQuestion("This room has one or more objects which use sprites from the current game. If you create a template of this room and then use it in another game, the object sprites will not match up. Do you want to continue?") == DialogResult.No)
					{
						createTemplate = false;
					}
					break;
				}
			}

			if (createTemplate)
			{
				_guiController.ShowCreateRoomTemplateWizard(roomToTemplatize);
			}
		}

		private List<RoomTemplate> ConstructListOfRoomTemplates()
		{
			List<RoomTemplate> templates = new List<RoomTemplate>();
			templates.Add(new RoomTemplate(null, null, "Blank Room"));
            foreach (string fileName in Utilities.GetDirectoryFileList(_agsEditor.TemplatesDirectory, "*.art", SearchOption.TopDirectoryOnly))
			{
				RoomTemplate template = _nativeProxy.LoadRoomTemplateFile(fileName);
				if (template != null)
				{
					templates.Add(template);
				}
			}
			return templates;
		}

		private void DisposeIcons(List<RoomTemplate> templates)
		{
			foreach (RoomTemplate template in templates)
			{
				if (template.Icon != null)
				{
					template.Icon.Dispose();
				}
			}
		}

		private void CloseRoomScriptEditorIfOpen(int roomNumber)
		{
			if (_roomScriptEditors.ContainsKey(roomNumber))
			{
				DisposePane(_roomScriptEditors[roomNumber]);
				_roomScriptEditors.Remove(roomNumber);
			}
		}

        private UnloadedRoom RemoveRoomFromGame(int roomNumber)
        {
            UnloadedRoom roomToDelete = FindRoomByID(roomNumber);
            if ((_loadedRoom != null) && (roomToDelete.Number == _loadedRoom.Number))
            {
                UnloadCurrentRoom();
            }
            _agsEditor.CurrentGame.Rooms.Remove(roomToDelete);

            RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
            RePopulateTreeView();
            return roomToDelete;
        }

        private void ImportExistingRoomFile(string fileName)
        {
            bool selectedFileInGameDirectory = false;
            if (Path.GetDirectoryName(fileName).ToLower() == _agsEditor.GameDirectory.ToLower())
            {
                selectedFileInGameDirectory = true;
            }

            int newRoomNumber, fileRoomNumber = -1;
            string fileNameWithoutPath = Path.GetFileName(fileName);
            Match match = Regex.Match(fileNameWithoutPath, @"room(\d+).crm", RegexOptions.IgnoreCase);
            if ((match.Success) && (match.Groups.Count == 2))
            {
                fileRoomNumber = Convert.ToInt32(match.Groups[1].Captures[0].Value);
                if (_agsEditor.CurrentGame.DoesRoomNumberAlreadyExist(fileRoomNumber))
                {
                    if (selectedFileInGameDirectory)
                    {
                        _guiController.ShowMessage("This file is already part of this game.", MessageBoxIcon.Information);
                        return;
                    }
                    _guiController.ShowMessage("This game already has a room " + fileRoomNumber + ". If you are sure that you want to import this file, rename it first.", MessageBoxIcon.Warning);
                    return;
                }
                newRoomNumber = fileRoomNumber;
            }
            else
            {
                newRoomNumber = _agsEditor.CurrentGame.FindFirstAvailableRoomNumber(0);
            }

            try
            {
                string newFileName = string.Format("room{0}.crm", newRoomNumber);
                string newScriptName = Path.ChangeExtension(newFileName, ".asc");

                if (selectedFileInGameDirectory)
                {
                    if (!File.Exists(newScriptName))
                    {
                        CopyScriptOutOfOldRoomFile(fileName, newScriptName);
                    }
					if (newRoomNumber != fileRoomNumber)
					{
						File.Copy(fileName, newFileName, true);
					}
                }
                else
                {
                    File.Copy(fileName, newFileName, true);

                    if (File.Exists(Path.ChangeExtension(fileName, ".asc")))
                    {
                        File.Copy(Path.ChangeExtension(fileName, ".asc"), newScriptName, true);
                    }
                    else
                    {
                        CopyScriptOutOfOldRoomFile(fileName, newScriptName);
                    }
                }

                UnloadedRoom newRoom = new UnloadedRoom(newRoomNumber);
                _agsEditor.CurrentGame.Rooms.Add(newRoom);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;

                RePopulateTreeView();
                RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
                _guiController.ShowMessage("Room file imported successfully as room " + newRoomNumber + ".", MessageBoxIcon.Information);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("There was a problem importing the room file: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void CopyScriptOutOfOldRoomFile(string roomFile, string scriptFile)
        {
            try
            {
                string roomScript = _nativeProxy.LoadRoomScript(roomFile);
                if (roomScript == null)
                {
                    roomScript = "// room script file\r\n";
                }
                StreamWriter sw = new StreamWriter(scriptFile, false, Encoding.Default);
                sw.Write(roomScript);
                sw.Close();
            }
            catch (AGSEditorException ex)
            {
                _guiController.ShowMessage("There was an error saving the script for room " + roomFile + ": " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private bool PromptForAndDeleteAnyExistingRoomFile(string fileName)
        {
            if (File.Exists(fileName))
            {
                if (_guiController.ShowQuestion("A file called '" + fileName + "' already exists. Do you want to overwrite it?") == DialogResult.No)
                {
                    return false;
                }
                try
                {
                    _agsEditor.AttemptToDeleteFileFromDisk(fileName);
                }
                catch (CannotDeleteFileException ex)
                {
                    _guiController.ShowMessage("The existing file could not be deleted." + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
                    return false;
                }
            }
            return true;
        }

        private void CreateNewRoom(int roomNumber, RoomTemplate template)
        {
            UnloadedRoom newRoom = new UnloadedRoom(roomNumber);
            if (!PromptForAndDeleteAnyExistingRoomFile(newRoom.FileName))
            {
                return;
            }

            try
            {
				if (template.FileName == null)
				{
					Resources.ResourceManager.CopyFileFromResourcesToDisk("blank.crm", newRoom.FileName);
					StreamWriter sw = new StreamWriter(newRoom.ScriptFileName);
					sw.WriteLine("// room script file");
					sw.Close();
				}
				else
				{
					_nativeProxy.ExtractRoomTemplateFiles(template.FileName, newRoom.Number);
				}

                _agsEditor.CurrentGame.Rooms.Add(newRoom);
                _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
				AddTreeNodeForRoom(newRoom);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
//                _guiController.ProjectTree.AddTreeLeaf(this, TREE_PREFIX_ROOM_NODE + newRoom.Number, newRoom.Number.ToString() + ": ", ROOM_ICON_UNLOADED);
                _guiController.ProjectTree.SelectNode(this, TREE_PREFIX_ROOM_NODE + newRoom.Number);
                RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("There was an error attempting to create the new room. The error was: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void SaveRoomButDoNotShowAnyErrors(Room room, CompileMessages errors, string pleaseWaitText)
        {
			lock (_roomLoadingOrSavingLock)
			{
				if (room != _loadedRoom)
				{
					errors.Add(new CompileError("Attempted to save room " + room.Number + " which is not loaded"));
					return;
				}

				PerformPreSaveChecks(room);

				if (!_agsEditor.AttemptToGetWriteAccess(room.FileName))
				{
					errors.Add(new CompileError("Unable to open file " + room.FileName + " for writing"));
					return;
				}

                if (PreSaveRoom != null)
                {
                    PreSaveRoom(room, errors);
                }

				EnsureScriptNamesAreUnique(room, errors);
                try
                {
                    if (!errors.HasErrors)
                    {
                        BusyDialog.Show(pleaseWaitText, new BusyDialog.ProcessingHandler(SaveRoomOnThread), room);
                    }
                }
                catch (CompileMessage ex)
                {
                    errors.Add(ex);
                }
                catch (AGSEditorException ex)
                {
                    errors.Add(new CompileError(ex.Message, ex));
                }
			}
        }

        private bool SaveRoomAndShowAnyErrors(Room room)
        {
            CompileMessages errors = new CompileMessages();

            if (_roomScriptEditors.ContainsKey(room.Number))
            {
                ((ScriptEditor)_roomScriptEditors[room.Number].Control).SaveChanges();
            }

			SaveRoomButDoNotShowAnyErrors(room, errors, "Please wait while the room is saved...");

            _guiController.ShowOutputPanel(errors);

			if (errors.HasErrors)
			{
				Factory.GUIController.ShowMessage("There were errors saving the room. Please consult the output window for details.", MessageBoxIcon.Warning);
			}

            return !errors.HasErrors;
        }

        private void EnsureScriptNamesAreUnique(Room room, CompileMessages errors)
        {
            foreach (RoomHotspot hotspot in room.Hotspots)
            {
                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(hotspot.Name, hotspot))
                {
                    errors.Add(new CompileError("Hotspot '" + hotspot.Name + "' script name conflicts with other game item"));
                }
            }
            foreach (RoomObject obj in room.Objects)
            {
                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(obj.Name, obj))
                {
                    errors.Add(new CompileError("Object '" + obj.Name + "' script name conflicts with other game item"));
                }
            }
        }

        private void PerformPreSaveChecks(Room room)
        {
            foreach (RoomWalkableArea area in room.WalkableAreas) 
            {
                if (area.UseContinuousScaling)
                {
                    if (area.MaxScalingLevel < area.MinScalingLevel)
                    {
                        int temp = area.MaxScalingLevel;
                        area.MaxScalingLevel = area.MinScalingLevel;
                        area.MinScalingLevel = temp;
                    }
                }
            }
            room.GameID = _agsEditor.CurrentGame.Settings.UniqueID;
        }

        private object SaveRoomOnThread(object parameter)
        {
            Room room = (Room)parameter;
            _agsEditor.RegenerateScriptHeader(room);
            List<Script> headers = (List<Script>)_agsEditor.GetAllScriptHeaders();
			_agsEditor.CompileScript(room.Script, headers, null, true);

            _nativeProxy.SaveRoom(room);
            room.Modified = false;
            return null;
        }

        private IScriptEditor GetScriptEditor(string fileName, bool showEditor)
        {
            int roomNumberToEdit = GetRoomNumberForFileName(fileName, false);

            if (roomNumberToEdit >= 0)
            {
                UnloadedRoom roomToGetScriptFor = GetUnloadedRoom(roomNumberToEdit);
                ScriptEditor editor = (ScriptEditor)GetScriptEditor(roomToGetScriptFor).Control;
                if (showEditor && editor != null)
                {
                    _guiController.AddOrShowPane(_roomScriptEditors[roomNumberToEdit]);            
                }
                return editor;
            }
            return null;
        }        

        private ContentDocument GetScriptEditor(UnloadedRoom selectedRoom)
        {
            if ((_roomScriptEditors.ContainsKey(selectedRoom.Number)) &&
                (!_roomScriptEditors[selectedRoom.Number].Visible))
            {
                DisposePane(_roomScriptEditors[selectedRoom.Number]);
                _roomScriptEditors.Remove(selectedRoom.Number);
            }

            if (!_roomScriptEditors.ContainsKey(selectedRoom.Number))
            {
                if (selectedRoom.Script == null)
                {
                    selectedRoom.LoadScript();
                }
                ScriptEditor scriptEditor = new ScriptEditor(selectedRoom.Script, _agsEditor);
                scriptEditor.RoomNumber = selectedRoom.Number;
                scriptEditor.IsModifiedChanged += new EventHandler(ScriptEditor_IsModifiedChanged);
                if ((_loadedRoom != null) && (_loadedRoom.Number == selectedRoom.Number))
                {
                    scriptEditor.Room = _loadedRoom;
                }
                _roomScriptEditors.Add(selectedRoom.Number, new ContentDocument(scriptEditor, selectedRoom.Script.FileName, this));
                _roomScriptEditors[selectedRoom.Number].ToolbarCommands = scriptEditor.ToolbarIcons;
                _roomScriptEditors[selectedRoom.Number].MainMenu = scriptEditor.ExtraMenu;
            }

            return _roomScriptEditors[selectedRoom.Number];
        }

        private ContentDocument CreateOrShowScript(UnloadedRoom selectedRoom)
        {
            ContentDocument scriptEditor = GetScriptEditor(selectedRoom);
            _guiController.AddOrShowPane(_roomScriptEditors[selectedRoom.Number]);
            return scriptEditor;
        }

        private void LoadRoom(string controlID)
        {
            UnloadedRoom selectedRoom = FindRoomByID(Convert.ToInt32(controlID.Substring(3)));
            if ((_loadedRoom == null) || (_roomSettings == null) ||
                (selectedRoom.Number != _loadedRoom.Number))
            {
                LoadDifferentRoom(selectedRoom);
            }
            if (_roomSettings != null)
            {
                _guiController.AddOrShowPane(_roomSettings);
            }
        }

        private void ScriptEditor_IsModifiedChanged(object sender, EventArgs e)
        {
            ScriptEditor sendingPane = (ScriptEditor)sender;
            string newTitle = sendingPane.Script.FileName + (sendingPane.IsModified ? " *" : "");

            foreach (ContentDocument doc in _roomScriptEditors.Values)
            {
                if (doc.Control == sendingPane)
                {
                    doc.Name = newTitle;
                    break;
                }
            }

            _guiController.DocumentTitlesChanged();
        }

        private delegate void DisposePaneDelegate(ContentDocument doc);

        private void DisposePane(ContentDocument doc)
        {
            if (doc != null)
            {
                if (doc.Control.InvokeRequired)
                {
                    doc.Control.Invoke(new DisposePaneDelegate(DisposePane), doc);
                }
                else
                {
                    _guiController.RemovePaneIfExists(doc);
                    doc.Dispose();
                }
            }
        }

        private void UnloadCurrentRoomAndGreyOutTree()
        {
            ProjectTree treeController = _guiController.ProjectTree;

            if (_loadedRoom != null)
            {
                if (_roomScriptEditors.ContainsKey(_loadedRoom.Number))
                {
                    ((ScriptEditor)_roomScriptEditors[_loadedRoom.Number].Control).Room = null;
                }

                treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_NODE + _loadedRoom.Number, ROOM_ICON_UNLOADED);
                treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_SETTINGS + _loadedRoom.Number, ROOM_ICON_UNLOADED);
            }

            UnloadCurrentRoom();
        }

		private void UnloadCurrentRoom()
		{
			if (_roomSettings != null)
			{
				((RoomSettingsEditor)_roomSettings.Control).SaveRoom -= new RoomSettingsEditor.SaveRoomHandler(RoomEditor_SaveRoom);
                ((RoomSettingsEditor)_roomSettings.Control).AbandonChanges -= new RoomSettingsEditor.AbandonChangesHandler(RoomsComponent_AbandonChanges);
            }
			DisposePane(_roomSettings);
			_roomSettings = null;

			if (_loadedRoom != null)
			{
				_loadedRoom.RoomModifiedChanged -= _modifiedChangedHandler;
			}
			_loadedRoom = null;
		}

        private Room LoadNewRoomIntoMemory(UnloadedRoom newRoom, CompileMessages errors)
        {
            if ((newRoom.Script == null) || (!newRoom.Script.Modified))
            {
				try
				{
					newRoom.LoadScript();
				}
				catch (FileNotFoundException)
				{
					_guiController.ShowMessage("The script file '" + newRoom.ScriptFileName + "' is missing. An empty script has been loaded instead.", MessageBoxIcon.Warning);
				}
            }
            else if (_roomScriptEditors.ContainsKey(newRoom.Number))
            {
                ((ScriptEditor)_roomScriptEditors[newRoom.Number].Control).UpdateScriptObjectWithLatestTextInWindow();
            }
            _loadedRoom = _nativeProxy.LoadRoom(newRoom);
            _loadedRoom.Modified = ImportExport.CreateInteractionScripts(_loadedRoom, errors);
            _loadedRoom.Modified |= HookUpInteractionVariables(_loadedRoom);
            _loadedRoom.Modified |= AddPlayMusicCommandToPlayerEntersRoomScript(_loadedRoom, errors);
			if (_loadedRoom.Script.Modified)
			{
				if (_roomScriptEditors.ContainsKey(_loadedRoom.Number))
				{
					((ScriptEditor)_roomScriptEditors[_loadedRoom.Number].Control).ScriptModifiedExternally();
				}
			}
            return _loadedRoom;
        }

        private bool AddPlayMusicCommandToPlayerEntersRoomScript(Room room, CompileMessages errors)
        {
            bool scriptModified = false;

            if (room.PlayMusicOnRoomLoad > 0)
            {
                AudioClip clip = _agsEditor.CurrentGame.FindAudioClipForOldMusicNumber(null, room.PlayMusicOnRoomLoad);
                if (clip == null)
                {
                    errors.Add(new CompileWarning("Room " + room.Number + ": Unable to find aMusic" + room.PlayMusicOnRoomLoad + " which was set as this room's start music"));
                    return scriptModified;
                }

                string scriptName = room.Interactions.GetScriptFunctionNameForInteractionSuffix(Room.EVENT_SUFFIX_ROOM_LOAD);
                if (string.IsNullOrEmpty(scriptName))
                {
                    scriptName = "Room_" + Room.EVENT_SUFFIX_ROOM_LOAD;
                    room.Interactions.SetScriptFunctionNameForInteractionSuffix(Room.EVENT_SUFFIX_ROOM_LOAD, scriptName);
                    room.Script.Text += Environment.NewLine + "function " + scriptName + "()" +
                        Environment.NewLine + "{" + Environment.NewLine +
                        "}" + Environment.NewLine;
                    scriptModified = true;
                }
                int functionOffset = room.Script.Text.IndexOf(scriptName);
                if (functionOffset < 0)
                {
                    errors.Add(new CompileWarning("Room " + room.Number + ": Unable to find definition for " + scriptName + " to add Room Load music " + room.PlayMusicOnRoomLoad));
                }
                else
                {
                    functionOffset = room.Script.Text.IndexOf('{', functionOffset);
                    functionOffset = room.Script.Text.IndexOf('\n', functionOffset) + 1;
                    string newScript = room.Script.Text.Substring(0, functionOffset);
                    newScript += "  " + clip.ScriptName + ".Play();" + Environment.NewLine;
                    newScript += room.Script.Text.Substring(functionOffset);
                    room.Script.Text = newScript;
                    room.PlayMusicOnRoomLoad = 0;
                    scriptModified = true;
                }
            }

            return scriptModified;
        }

        private bool HookUpInteractionVariables(Room room)
        {
            bool changedScript = false;
            int index;
            while ((index = room.Script.Text.IndexOf("__INTRVAL$")) > 0)
            {
                int endIndex = room.Script.Text.IndexOf('$', index + 10) + 1;
                string intrVariableNumber = room.Script.Text.Substring(index + 10, (endIndex - index) - 11);
                int variableNumber = Convert.ToInt32(intrVariableNumber);
                string replacementText;

                if (variableNumber >= OldInteractionVariable.LOCAL_VARIABLE_INDEX_OFFSET)
                {
                    variableNumber -= OldInteractionVariable.LOCAL_VARIABLE_INDEX_OFFSET;
                    if (variableNumber >= room.OldInteractionVariables.Count)
                    {
                        replacementText = "UNKNOWN_ROOM_VARIABLE_" + variableNumber;
                    }
                    else
                    {
                        replacementText = room.OldInteractionVariables[variableNumber].ScriptName;
                    }
                }
                else
                {
                    if (variableNumber >= _agsEditor.CurrentGame.OldInteractionVariables.Count)
                    {
                        replacementText = "UNKNOWN_INTERACTION_VARIABLE_" + variableNumber;
                    }
                    else
                    {
                        replacementText = _agsEditor.CurrentGame.OldInteractionVariables[variableNumber].ScriptName;
                    }
                }

                room.Script.Text = room.Script.Text.Replace(room.Script.Text.Substring(index, (endIndex - index)), replacementText);
                changedScript = true;
            }

			if ((changedScript) && (room.OldInteractionVariables.Count > 0))
			{
				foreach (OldInteractionVariable var in room.OldInteractionVariables)
				{
					room.Script.Text = string.Format("int {0} = {1};{2}", var.ScriptName, var.Value, Environment.NewLine) + room.Script.Text;
				}
				room.Script.Text = "// Automatically converted interaction variables" + Environment.NewLine + room.Script.Text;
			}

			return changedScript;
        }

        private bool LoadDifferentRoom(UnloadedRoom newRoom)
        {
            if ((_roomSettings != null) && (_roomSettings.Visible))
            {
                // give them a chance to save the current room first
                bool cancelLoad = false;
                ((RoomSettingsEditor)_roomSettings.Control).PanelClosing(true, ref cancelLoad);
                if (cancelLoad)
                {
                    return false;
                }
            }

			// lock to stop the load proceeding until a save has finished,
			// just to make sure we don't end up with a race condition if
			// the user is madly clicking around
			lock (_roomLoadingOrSavingLock)
			{
                UnloadCurrentRoomAndGreyOutTree();

				if (!File.Exists(newRoom.FileName))
				{
					_guiController.ShowMessage("The file '" + newRoom.FileName + "' was not found. Unable to open this room.", MessageBoxIcon.Warning);
				}
				else
				{
					CompileMessages errors = new CompileMessages();

					LoadNewRoomIntoMemory(newRoom, errors);

					_loadedRoom.RoomModifiedChanged += _modifiedChangedHandler;

					string paneTitle = "Room " + _loadedRoom.Number + (_loadedRoom.Modified ? " *" : "");

					_roomSettings = new ContentDocument(new RoomSettingsEditor(_loadedRoom), paneTitle, this, ConstructPropertyObjectList(_loadedRoom));
					_roomSettings.SelectedPropertyGridObject = _loadedRoom;
					((RoomSettingsEditor)_roomSettings.Control).SaveRoom += new RoomSettingsEditor.SaveRoomHandler(RoomEditor_SaveRoom);
                    ((RoomSettingsEditor)_roomSettings.Control).AbandonChanges += new RoomSettingsEditor.AbandonChangesHandler(RoomsComponent_AbandonChanges);

					if (_loadedRoom.Modified)
					{
						CreateOrShowScript(_loadedRoom);
					}

					if (_roomScriptEditors.ContainsKey(_loadedRoom.Number))
					{
						((ScriptEditor)_roomScriptEditors[_loadedRoom.Number].Control).Room = _loadedRoom;
						if (_loadedRoom.Modified)
						{
							((ScriptEditor)_roomScriptEditors[_loadedRoom.Number].Control).ScriptModifiedExternally();
						}
					}

                    ProjectTree treeController = _guiController.ProjectTree;
					treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_NODE + _loadedRoom.Number, ROOM_ICON_LOADED);
					treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_SETTINGS + _loadedRoom.Number, ROOM_ICON_LOADED);

					_guiController.ShowOutputPanel(errors);
				}

				return true;
			}
        }

        private void RoomsComponent_AbandonChanges(Room room)
        {
            if (room == _loadedRoom)
            {
                UnloadCurrentRoomAndGreyOutTree();
            }
        }

        private bool RoomEditor_SaveRoom(Room room)
        {
            return SaveRoomAndShowAnyErrors(room);
        }

        private void _loadedRoom_RoomModifiedChanged(bool isModified)
        {
            if (isModified)
            {
                // Prompt it to check out if necessary
                _agsEditor.AttemptToGetWriteAccess(_loadedRoom.FileName);
            }
            _roomSettings.Name = "Room " + _loadedRoom.Number + (isModified ? " *" : "");
            _guiController.DocumentTitlesChanged();
        }

        private UnloadedRoom FindRoomByID(int roomNumber)
        {
			UnloadedRoom room = _agsEditor.CurrentGame.FindRoomByID(roomNumber);
			if (room == null)
			{
				throw new AGSEditorException("Internal error: Room " + roomNumber + " not found in list");
			}
			return room;
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if ((propertyName == UnloadedRoom.PROPERTY_NAME_DESCRIPTION) && (_loadedRoom != null))
            {
                UnloadedRoom room = FindRoomByID(_loadedRoom.Number);
                room.Description = _loadedRoom.Description;
                RePopulateTreeView();
            }

			if ((propertyName == UnloadedRoom.PROPERTY_NAME_NUMBER) && (_loadedRoom != null))
			{
				int numberRequested = _loadedRoom.Number;
				_loadedRoom.Number = Convert.ToInt32(oldValue);

				RenameRoom(_loadedRoom.Number, numberRequested);
			}

        }

		private void RenameRoom(int currentNumber, int numberRequested)
		{
			UnloadedRoom room = _agsEditor.CurrentGame.FindRoomByID(numberRequested);
			if (room != null)
			{
				_guiController.ShowMessage("There is already a room number " + numberRequested + ". Please choose another number.", MessageBoxIcon.Warning);
			}
			else if ((numberRequested < 0) || (numberRequested > UnloadedRoom.HIGHEST_ROOM_NUMBER_ALLOWED))
			{
				_guiController.ShowMessage("The new room number must be between 0 and " + UnloadedRoom.HIGHEST_ROOM_NUMBER_ALLOWED + ".", MessageBoxIcon.Warning);
			}
			else if (UnloadedRoom.DoRoomFilesExist(numberRequested))
			{
				_guiController.ShowMessage("Your game directory already has an existing file with the target room number.", MessageBoxIcon.Warning);
			}
			else if (SaveRoomAndShowAnyErrors(_loadedRoom))
			{
				UnloadedRoom oldRoom = FindRoomByID(currentNumber);
				UnloadedRoom tempNewRoom = new UnloadedRoom(numberRequested);
				CloseRoomScriptEditorIfOpen(currentNumber);
				UnloadCurrentRoom();

				_agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldRoom.FileName, tempNewRoom.FileName);
				_agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldRoom.ScriptFileName, tempNewRoom.ScriptFileName);

				oldRoom.Number = numberRequested;

				LoadDifferentRoom(oldRoom);
				_guiController.AddOrShowPane(_roomSettings);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
				RePopulateTreeView();
			}
		}

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New room...", null));
                menu.Add(new MenuCommand(COMMAND_ADD_EXISTING_ROOM, "Import existing room...", null));
				menu.Add(MenuCommand.Separator);
				menu.Add(new MenuCommand(COMMAND_SORT_BY_NUMBER, "Sort rooms by number", null));
			}
            else if ((controlID.StartsWith(TREE_PREFIX_ROOM_SETTINGS)) ||
                     (controlID.StartsWith(TREE_PREFIX_ROOM_SCRIPT)))
            {
                // No right-click menu for these
            }
            else
            {
                _rightClickedRoomNumber = Convert.ToInt32(controlID.Substring(3));

                menu.Add(new MenuCommand(COMMAND_LOAD_ROOM, "Edit this room", null));
                menu.Add(MenuCommand.Separator);

                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this room", null));
                menu.Add(new MenuCommand(COMMAND_EXCLUDE_ITEM, "Exclude from game", null));
                menu.Add(MenuCommand.Separator);
                menu.Add(new MenuCommand(COMMAND_CREATE_TEMPLATE, "Create template from room...", null));
            }
            return menu;
        }

        public override void BeforeSaveGame()
        {
            foreach (ContentDocument doc in _roomScriptEditors.Values)
            {
                ((ScriptEditor)doc.Control).SaveChanges();
            }
        }

        public override void RefreshDataFromGame()
        {
            UnloadCurrentRoom();

            foreach (ContentDocument doc in _roomScriptEditors.Values)
            {
                DisposePane(doc);
            }
            _roomScriptEditors.Clear();

            RePopulateTreeView();
            RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
        }

        private int GetRoomNumberForFileName(string fileName, bool isDebugExecutionPoint)
        {
            if ((fileName == Script.CURRENT_ROOM_SCRIPT_FILE_NAME) &&
               (_loadedRoom != null))
            {
                fileName = _loadedRoom.ScriptFileName;
            }

            int roomNumberToEdit = -1;

            foreach (ContentDocument doc in _roomScriptEditors.Values)
            {
                if (isDebugExecutionPoint)
                {
                    ((ScriptEditor)doc.Control).RemoveExecutionPointMarker();
                }

                if (((ScriptEditor)doc.Control).Script.FileName == fileName)
                {
                    roomNumberToEdit = ((ScriptEditor)doc.Control).RoomNumber;
                }
            }

            if ((roomNumberToEdit < 0) && (_loadedRoom != null) &&
                (fileName == _loadedRoom.ScriptFileName))
            {
                roomNumberToEdit = _loadedRoom.Number;
            }

            if (roomNumberToEdit < 0)
            {
                Match match = Regex.Match(fileName, @"^room(\d+)\.asc$", RegexOptions.IgnoreCase);
                if ((match.Success) && (match.Groups.Count == 2))
                {
                    roomNumberToEdit = Convert.ToInt32(match.Groups[1].Captures[0].Value);
                }
            }

            return roomNumberToEdit;
        }

        private UnloadedRoom GetUnloadedRoom(int roomNumber)
        {
            UnloadedRoom roomToGetScriptFor;
            if ((_loadedRoom != null) && (roomNumber == _loadedRoom.Number))
            {
                roomToGetScriptFor = _loadedRoom;
            }
            else
            {
                roomToGetScriptFor = FindRoomByID(roomNumber);
            }
            return roomToGetScriptFor;
        }

        private void GUIController_OnZoomToFile(ZoomToFileEventArgs evArgs)
        {
            int roomNumberToEdit = GetRoomNumberForFileName(evArgs.FileName, evArgs.IsDebugExecutionPoint);
            
            if (roomNumberToEdit >= 0)
            {
                UnloadedRoom roomToGetScriptFor = GetUnloadedRoom(roomNumberToEdit);                
                ScriptEditor editor = (ScriptEditor)CreateOrShowScript(roomToGetScriptFor).Control;
				ZoomToCorrectPositionInScript(editor, evArgs);
            }
        }        

        private void GUIController_OnGetScript(string fileName, ref Script script)
        {
            if ((fileName == Script.CURRENT_ROOM_SCRIPT_FILE_NAME) &&
                (_loadedRoom != null))
            {
                if (_roomScriptEditors.ContainsKey(_loadedRoom.Number))
                {
                    ((ScriptEditor)_roomScriptEditors[_loadedRoom.Number].Control).UpdateScriptObjectWithLatestTextInWindow();
                }
                script = _loadedRoom.Script;
            }
        }

        private void GUIController_OnScriptChanged(Script script)
        {
            if ((_loadedRoom != null) &&
                (script == _loadedRoom.Script) &&
                (_roomScriptEditors.ContainsKey(_loadedRoom.Number)))
            {
                ((ScriptEditor)_roomScriptEditors[_loadedRoom.Number].Control).ScriptModifiedExternally();
            }
        }

		/// <summary>
		/// Determines whether any script headers are newer than the room.
		/// This is important because changes to any function declarations or #defines
		/// in the header files need to be reflected in the script compilation.
		/// </summary>
		/// <param name="roomFileName">Room .CRM file name</param>
		private bool HaveScriptHeadersBeenUpdatedSinceRoomWasCompiled(string roomFileName)
		{
			foreach (Script script in _agsEditor.CurrentGame.Scripts)
			{
				if ((script.IsHeader) &&
					(Utilities.DoesFileNeedRecompile(script.FileName, roomFileName)))
				{
					return true;
				}
			}
			return false;
		}

        private bool RecompileAnyRoomsWhereTheScriptHasChanged(CompileMessages errors, bool rebuildAll)
        {
			List<UnloadedRoom> roomsToRebuild = new List<UnloadedRoom>();
			List<string> roomFileNamesToRebuild = new List<string>();
            bool success = true;
            foreach (UnloadedRoom unloadedRoom in _agsEditor.CurrentGame.Rooms)
            {
                if (!File.Exists(unloadedRoom.ScriptFileName))
                {
                    errors.Add(new CompileError("File not found: " + unloadedRoom.ScriptFileName + "; If you deleted this file, use the Exclude From Game option to remove it from the game."));
                    success = false;
                }
                else if (!File.Exists(unloadedRoom.FileName))
                {
                    errors.Add(new CompileError("File not found: " + unloadedRoom.FileName + "; If you deleted this file, use the Exclude From Game option to remove it from the game."));
                    success = false;
                }
                else if ((rebuildAll) ||
					(Utilities.DoesFileNeedRecompile(unloadedRoom.ScriptFileName, unloadedRoom.FileName)) ||
					(HaveScriptHeadersBeenUpdatedSinceRoomWasCompiled(unloadedRoom.FileName)))
                {
					roomsToRebuild.Add(unloadedRoom);
					roomFileNamesToRebuild.Add(unloadedRoom.FileName);
                }
            }

			if (!_agsEditor.AttemptToGetWriteAccess(roomFileNamesToRebuild))
			{
				errors.Add(new CompileError("Failed to open files for writing"));
				return false;
			}

			foreach (UnloadedRoom unloadedRoom in roomsToRebuild)
			{
				Room room;
				if ((_loadedRoom == null) || (_loadedRoom.Number != unloadedRoom.Number))
				{
					UnloadCurrentRoom();
					room = LoadNewRoomIntoMemory(unloadedRoom, errors);
				}
				else
				{
					room = _loadedRoom;
				}
				// Ensure that the script is saved (in case this is a 2.72
				// room and LoadNewRoom has just jibbled the script)
				room.Script.SaveToDisk();

				CompileMessages roomErrors = new CompileMessages();
				SaveRoomButDoNotShowAnyErrors(room, roomErrors, "Rebuilding room " + room.Number + " because a script has changed...");

				if (roomErrors.Count > 0)
				{
					errors.Add(new CompileError("Failed to save room " + room.FileName + "; details below"));
					errors.Add(roomErrors[0]);
					success = false;
					break;
				}
			}

            return success;
        }

		private void AGSEditor_PreDeleteSprite(PreDeleteSpriteEventArgs evArgs)
		{
			if (_loadedRoom != null)
			{
				foreach (RoomObject obj in _loadedRoom.Objects)
				{
					if (obj.Image == evArgs.SpriteNumber)
					{
						_guiController.ShowMessage("Sprite " + evArgs.SpriteNumber + " is in use by object " + obj.ID + " in this room and cannot be deleted.", MessageBoxIcon.Warning);
						evArgs.AllowDelete = false;
						return;
					}
				}
			}
		}

        private void AGSEditor_PreCompileGame(PreCompileGameEventArgs evArgs)
        {
            if ((_loadedRoom != null) && (_loadedRoom.Modified))
            {
                evArgs.AllowCompilation = SaveRoomAndShowAnyErrors(_loadedRoom);
            }

            if (evArgs.AllowCompilation)
            {
				evArgs.AllowCompilation = RecompileAnyRoomsWhereTheScriptHasChanged(evArgs.Errors, evArgs.ForceRebuild);
            }
        }

        private void AGSEditor_PreSaveGame(PreSaveGameEventArgs evArgs)
        {
            if ((_loadedRoom != null) && (_loadedRoom.Modified))
            {
				if (!SaveRoomAndShowAnyErrors(_loadedRoom))
				{
					evArgs.SaveSucceeded = false;
				}
            }
        }

		private bool CheckOutAllRoomsAndScripts()
		{
			List<string> roomFileNamesToRebuild = new List<string>();
			foreach (UnloadedRoom unloadedRoom in _agsEditor.CurrentGame.Rooms)
			{
				roomFileNamesToRebuild.Add(unloadedRoom.FileName);
				roomFileNamesToRebuild.Add(unloadedRoom.ScriptFileName);
			}

			return _agsEditor.AttemptToGetWriteAccess(roomFileNamesToRebuild);
		}

        private void AGSEditor_ProcessAllGameTexts(IGameTextProcessor processor, CompileMessages errors)
        {
			if (processor.MakesChanges)
			{
				if (!CheckOutAllRoomsAndScripts())
				{
					errors.Add(new CompileError("Failed to open room files for writing"));
					return;
				}
			}

			foreach (UnloadedRoom unloadedRoom in _agsEditor.CurrentGame.Rooms)
            {
                UnloadCurrentRoom();
                Room room = LoadNewRoomIntoMemory(unloadedRoom, errors);

                room.Script.Text = processor.ProcessText(room.Script.Text, GameTextType.Script);
                if (processor.MakesChanges)
                {
                    room.Script.SaveToDisk();
                }

                foreach (RoomMessage message in room.Messages)
                {
                    int charId = message.CharacterID;
                    if (!message.ShowAsSpeech)
                    {
                        charId = Character.NARRATOR_CHARACTER_ID;
                    }
                    message.Text = processor.ProcessText(message.Text, GameTextType.Message, charId);
                }

                foreach (RoomHotspot hotspot in room.Hotspots)
                {
                    hotspot.Description = processor.ProcessText(hotspot.Description, GameTextType.ItemDescription);
                }

                foreach (RoomObject obj in room.Objects)
                {
                    obj.Description = processor.ProcessText(obj.Description, GameTextType.ItemDescription);
                }

                if (processor.MakesChanges)
                {
                    CompileMessages roomErrors = new CompileMessages();
					SaveRoomButDoNotShowAnyErrors(room, roomErrors, "Saving updates to room " + room.Number + "...");

                    if (roomErrors.Count > 0)
                    {
                        errors.Add(new CompileError("Failed to save room " + room.FileName + "; details below"));
                        errors.Add(roomErrors[0]);
                    }
                }
            }
        }

		private void AddTreeNodeForRoom(UnloadedRoom room)
		{
			string iconName = ROOM_ICON_UNLOADED;

			if ((_loadedRoom != null) && (room.Number == _loadedRoom.Number))
			{
				iconName = ROOM_ICON_LOADED;
			}

			ProjectTree treeController = _guiController.ProjectTree;
			treeController.AddTreeBranch(this, TREE_PREFIX_ROOM_NODE + room.Number, room.Number.ToString() + ": " + room.Description, iconName);
			treeController.AddTreeLeaf(this, TREE_PREFIX_ROOM_SETTINGS + room.Number, "Edit room", iconName);
			treeController.AddTreeLeaf(this, TREE_PREFIX_ROOM_SCRIPT + room.Number, "Room script", "ScriptIcon");
		}

        private void RePopulateTreeView()
        {
			ProjectTree treeController = _guiController.ProjectTree;
			treeController.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            treeController.StartFromNode(this, TOP_LEVEL_COMMAND_ID);

            foreach (UnloadedRoom room in _agsEditor.CurrentGame.Rooms)
            {
				AddTreeNodeForRoom(room);
                treeController.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            }

            if (_loadedRoom != null)
            {
                treeController.SelectNode(this, TREE_PREFIX_ROOM_SETTINGS + _loadedRoom.Number);
            }

/*            if (_documents.ContainsValue(_guiController.ActivePane))
            {
                CharacterEditor editor = (CharacterEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, "Chr" + editor.ItemToEdit.ID);
            }
            else if (_agsEditor.CurrentGame.Rooms.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "Rom0");
            }*/
        }

        private Dictionary<string, object> ConstructPropertyObjectList(Room room)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(room.PropertyGridTitle, room);
            return list;
        }

		ILoadedRoom IRoomController.CurrentRoom
		{
			get { return _loadedRoom; }
		}

		bool IRoomController.LoadRoom(IRoom roomToLoad)
		{
			if ((_loadedRoom != null) && (_loadedRoom.Number == roomToLoad.Number))
			{
				return true;
			}
			return LoadDifferentRoom((UnloadedRoom)roomToLoad);
		}

		int IRoomController.GetAreaMaskPixel(RoomAreaMaskType maskType, int x, int y)
		{
			if (_loadedRoom == null)
			{
				throw new InvalidOperationException("No room is currently loaded");
			}
			return _nativeProxy.GetAreaMaskPixel(_loadedRoom, maskType, x, y);
		}

		void IRoomController.DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, int scaleFactor)
		{
			((IRoomController)this).DrawRoomBackground(g, x, y, backgroundNumber, scaleFactor, RoomAreaMaskType.None, 0, 0);
		}

		void IRoomController.DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, int scaleFactor, RoomAreaMaskType maskType, int maskTransparency, int selectedArea)
		{
			if (_loadedRoom == null)
			{
				throw new InvalidOperationException("No room is currently loaded");
			}
			if ((maskTransparency < 0) || (maskTransparency > 100))
			{
				throw new ArgumentOutOfRangeException("maskTransparency", "Mask Transparency must be between 0 and 100");
			}
			_nativeProxy.CreateBuffer((int)g.VisibleClipBounds.Width, (int)g.VisibleClipBounds.Height);
			IntPtr hdc = g.GetHdc();
			_nativeProxy.DrawRoomBackground(hdc, _loadedRoom, x, y, backgroundNumber, scaleFactor, maskType, selectedArea, maskTransparency);
			_nativeProxy.RenderBufferToHDC(hdc);
			g.ReleaseHdc(hdc);
		}

		bool IRoomController.GreyOutNonSelectedMasks
		{
			set { _nativeProxy.GreyOutNonSelectedMasks = value; }
		}
	}
}
