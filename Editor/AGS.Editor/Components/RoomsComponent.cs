using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;
using WeifenLuo.WinFormsUI.Docking;
using System.Threading;
using System.Linq;
using System.Drawing.Imaging;
using System.Threading.Tasks;
using System.Xml.Linq;
using AGS.Types.Interfaces;

namespace AGS.Editor.Components
{
    class RoomsComponent : BaseComponentWithScripts<IRoom, UnloadedRoomFolder>, ISaveable, IDisposable, IRoomController
    {
        private const string ROOMS_COMMAND_ID = "Rooms";
        private const string COMMAND_NEW_ITEM = "NewRoom";
        private const string COMMAND_IMPORT_ROOM = "AddExistingRoom";
		private const string COMMAND_SORT_BY_NUMBER = "SortByNumber";
        private const string COMMAND_DELETE_ITEM = "DeleteRoom";        
		private const string COMMAND_CREATE_TEMPLATE = "TemplateFromRoom";
        private const string TREE_PREFIX_ROOM_NODE = "Rom";
        private const string TREE_PREFIX_ROOM_SETTINGS = "Roe";
        private const string TREE_PREFIX_ROOM_SCRIPT = "Ros";
        private const string COMMAND_LOAD_ROOM = "LoadRoom";
        private const string COMMAND_SAVE_ROOM = "SaveRoom";

        private const string ROOM_ICON_UNLOADED = "RoomIcon";
        private const string ROOM_ICON_LOADED = "RoomColourIcon";
        private const string SCRIPT_ICON = "ScriptIcon";

        private readonly List<Bitmap> _backgroundCache = new List<Bitmap>(Room.MAX_BACKGROUNDS);
        private readonly Dictionary<RoomAreaMaskType, Bitmap> _maskCache = new Dictionary<RoomAreaMaskType, Bitmap>(Enum.GetValues(typeof(RoomAreaMaskType)).Length);
        private readonly FileWatcherCollection _fileWatchers = new FileWatcherCollection();

        public event PreSaveRoomHandler PreSaveRoom;
        private ContentDocument _roomSettings;
        private Dictionary<int,ContentDocument> _roomScriptEditors = new Dictionary<int,ContentDocument>();
        private Room _loadedRoom;
		private NativeProxy _nativeProxy;
        private int _rightClickedRoomNumber;
        private Room.RoomModifiedChangedHandler _modifiedChangedHandler;
		private object _roomLoadingOrSavingLock = new object();
        private bool _grayOutMasks = true;

        public RoomsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, ROOMS_COMMAND_ID)
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
            Factory.Events.GamePostLoad += ConvertAllRoomsFromCrmToOpenFormat;
            _modifiedChangedHandler = new Room.RoomModifiedChangedHandler(_loadedRoom_RoomModifiedChanged);
            RePopulateTreeView();
        }

        public void Dispose()
        {
            ClearImageCache();
            _fileWatchers.Dispose();
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

        public bool IsBeingSaved { get; private set; }

        public DateTime LastSavedAt { get; private set; }

        protected override void ItemCommandClick(string controlID)
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
            else if (controlID == COMMAND_IMPORT_ROOM)
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
			else if (controlID == COMMAND_DELETE_ITEM)
			{
				if (MessageBox.Show("Are you sure you want to delete this room? It cannot be recovered.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
				{
                    if (EnsureNoCharactersStartInRoom(_rightClickedRoomNumber))
                    {
                        DeleteSingleItem(FindRoomByID(_rightClickedRoomNumber));
                    }
				}
			}
			else if (controlID == COMMAND_SORT_BY_NUMBER)
			{
                _agsEditor.CurrentGame.RootRoomFolder.Sort(true);				
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
                    _roomSettings.TreeNodeID = TREE_PREFIX_ROOM_SETTINGS + _rightClickedRoomNumber;
					_guiController.AddOrShowPane(_roomSettings);
				}
			}
			else if (controlID == COMMAND_SAVE_ROOM)
			{
				if (_loadedRoom != null)
				{
                    SaveRoomIfModifiedAndShowErrors(_loadedRoom, _roomSettings?.Control as RoomSettingsEditor);
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
            foreach (var character in _agsEditor.CurrentGame.RootCharacterFolder.AllItemsFlat)
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

        protected override void DeleteResourcesUsedByItem(IRoom item)
        {
            DeleteRoom(item);
        }

        private void DeleteRoom(IRoom roomToDelete)
        {
            UnloadRoom(roomToDelete);

            CloseRoomScriptEditorIfOpen(roomToDelete.Number);

            List<string> filesToDelete = new List<string>();

            if (File.Exists(roomToDelete.FileName))
            {
                filesToDelete.Add(roomToDelete.FileName);
            }
            if (File.Exists(roomToDelete.ScriptFileName))
            {
                filesToDelete.Add(roomToDelete.ScriptFileName);
            }
            if (File.Exists(roomToDelete.UserFileName))
            {
                filesToDelete.Add(roomToDelete.UserFileName);
            }
            if (File.Exists(roomToDelete.DataFileName))
            {
                filesToDelete.Add(roomToDelete.DataFileName);
            }

            var backgroundFiles = Enumerable.Range(0, Room.MAX_BACKGROUNDS)
                .Select(i => roomToDelete.GetBackgroundFileName(i))
                .Where(f => File.Exists(f));
            filesToDelete.AddRange(backgroundFiles);

            var maskFiles = Enum.GetValues(typeof(RoomAreaMaskType))
                .Cast<RoomAreaMaskType>()
                .Where(m => m != RoomAreaMaskType.None)
                .Select(m => roomToDelete.GetMaskFileName(m))
                .Where(f => File.Exists(f));
            filesToDelete.AddRange(maskFiles);

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
			if (_loadedRoom != null)
			{
                if (!SaveRoomOnlyGameDataAndShowErrors(_loadedRoom))
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
				_guiController.SaveRoomAsTemplate(roomToTemplatize);
			}
		}

        private List<RoomTemplate> ConstructListOfRoomTemplates()
        {
            List<RoomTemplate> templates = new List<RoomTemplate>();
            templates.Add(new RoomTemplate(null, null, "Blank Room"));
            string[] directories = new string[] { _agsEditor.TemplatesDirectory, _agsEditor.UserTemplatesDirectory };

            foreach (string directory in directories)
            {
                foreach (string fileName in Utilities.GetDirectoryFileList(directory, "*.art", SearchOption.TopDirectoryOnly))
                {
                    RoomTemplate template = _nativeProxy.LoadRoomTemplateFile(fileName);

                    if (template != null)
                    {
                        templates.Add(template);
                    }
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
            ContentDocument document;
			if (_roomScriptEditors.TryGetValue(roomNumber, out document))
			{
                DisposePane(document);
				_roomScriptEditors.Remove(roomNumber);
			}
		}

        private void UnloadRoom(IRoom roomToDelete)
        {
            if ((_loadedRoom != null) && (roomToDelete.Number == _loadedRoom.Number))
            {
                UnloadCurrentRoom();
            }
            
            RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);            
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
                Task.WaitAll(ConvertRoomFromCrmToOpenFormat(newRoom, null).ToArray());
                AddSingleItem(newRoom);                
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
                // NOTE: old game format: texts are always ANSI/ASCII
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
                    Utilities.TryDeleteFile(fileName);
                }
                catch (Exception ex)
                {
                    _guiController.ShowMessage("The existing file could not be deleted." + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
                    return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Creates empty room definition, not attached to any actual resources yet.
        /// </summary>
        private Room CreateEmptyRoom(int roomNumber)
        {
            Room room = new Room(roomNumber);
            var gameSettings = _agsEditor.CurrentGame.Settings;
            room.GameID = gameSettings.UniqueID;
            room.Width = gameSettings.CustomResolution.Width;
            room.Height = gameSettings.CustomResolution.Height;
            room.MaskResolution = gameSettings.DefaultRoomMaskResolution;
            room.ColorDepth = (int)gameSettings.ColorDepth * 8; // from bytes to bits per pixel
            room.BackgroundCount = 1;
            room.RightEdgeX = room.Width - 1;
            room.BottomEdgeY = room.Height - 1;
            return room;
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
                    // Create a default room and save it, generating clear backgrounds and masks
					Room room = CreateEmptyRoom(roomNumber);
					Factory.NativeProxy.SaveDefaultRoom(room);
					StreamWriter sw = new StreamWriter(newRoom.ScriptFileName);
					sw.WriteLine("// room script file");
					sw.Close();
				}
				else
				{
					_nativeProxy.ExtractRoomTemplateFiles(template.FileName, newRoom.Number);
				}


                Task.WaitAll(ConvertRoomFromCrmToOpenFormat(newRoom, null, template.FileName == null).ToArray());

                string newNodeID = AddSingleItem(newRoom);
                _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                _guiController.ProjectTree.SelectNode(this, newNodeID);
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

        private bool SaveRoomComponentsAndShowErrors(Room room, RoomSettingsEditor editor, bool onlyIfModified)
        {
            CompileMessages errors = new CompileMessages();

            // Save design-time room preferences. Currently they are stored directly
            // in the editor class, therefore we need an instance on one to do this.
            if (editor != null && (editor.DesignModified || !onlyIfModified))
            {
                RoomDesignData.SaveToUserFile(_loadedRoom, editor, errors);
                editor.DesignModified = false;
            }

            if (room != null && (room.Modified || !onlyIfModified))
            {
                if (_roomScriptEditors.ContainsKey(room.Number))
                {
                    ((ScriptEditor)_roomScriptEditors[room.Number].Control).SaveChanges();
                }

                SaveRoomButDoNotShowAnyErrors(room, errors, "Please wait while the room is saved...");
            }

            _guiController.ShowOutputPanel(errors);

            if (errors.HasErrors)
            {
                Factory.GUIController.ShowMessage("There were errors or warnings when saving the room. Please consult the output window for details.", MessageBoxIcon.Warning);
            }

            return !errors.HasErrors;
        }

        private bool SaveRoomAlwaysAndShowErrors(Room room, RoomSettingsEditor editor)
        {
            return SaveRoomComponentsAndShowErrors(room, editor, false);
        }

        private bool SaveRoomIfModifiedAndShowErrors(Room room, RoomSettingsEditor editor)
        {
            return SaveRoomComponentsAndShowErrors(room, editor, true);
        }

        private bool SaveRoomOnlyGameDataAndShowErrors(Room room)
        {
            return SaveRoomComponentsAndShowErrors(room, null, false);
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

        private object SaveRoomOnThread(IWorkProgress progress, object parameter)
        {            
            Room room = (Room)parameter;
            _agsEditor.RegenerateScriptHeader(room);
            List<Script> headers = (List<Script>)_agsEditor.GetAllScriptHeaders();
            CompileMessages messages = new CompileMessages();
            _agsEditor.CompileScript(room.Script, headers, messages);
            if (messages.HasErrors)
                throw messages.Errors[0];

            ((IRoomController)this).Save();

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
                    ContentDocument document = _roomScriptEditors[roomNumberToEdit];
                    document.TreeNodeID = TREE_PREFIX_ROOM_SCRIPT + roomNumberToEdit;
                    _guiController.AddOrShowPane(document);            
                }
                return editor;
            }
            return null;
        }        

        private ContentDocument GetScriptEditor(UnloadedRoom selectedRoom)
        {
            ContentDocument document;
            if (_roomScriptEditors.TryGetValue(selectedRoom.Number, out document) && !document.Visible)
            {
                DisposePane(document);
                _roomScriptEditors.Remove(selectedRoom.Number);
            }

            if (!_roomScriptEditors.TryGetValue(selectedRoom.Number, out document) || document.Control.IsDisposed)
            {
                if (selectedRoom.Script == null)
                {
                    selectedRoom.LoadScript();
                }
                CreateScriptEditor(selectedRoom);
            }

            return _roomScriptEditors[selectedRoom.Number];
        }

        private void CreateScriptEditor(UnloadedRoom selectedRoom)
        {
            ScriptEditor scriptEditor = new ScriptEditor(selectedRoom.Script, _agsEditor, null);
            scriptEditor.RoomNumber = selectedRoom.Number;
            scriptEditor.IsModifiedChanged += new EventHandler(ScriptEditor_IsModifiedChanged);
            if (scriptEditor.DockingContainer == null)
            {
                scriptEditor.DockingContainer = new DockingContainer(scriptEditor);
            }
            if ((_loadedRoom != null) && (_loadedRoom.Number == selectedRoom.Number))
            {
                scriptEditor.Room = _loadedRoom;
            }
            _roomScriptEditors[selectedRoom.Number] = new ContentDocument(scriptEditor,
                selectedRoom.Script.FileNameWithoutPath, this, SCRIPT_ICON);
            _roomScriptEditors[selectedRoom.Number].ToolbarCommands = scriptEditor.ToolbarIcons;
            _roomScriptEditors[selectedRoom.Number].MainMenu = scriptEditor.ExtraMenu; 
        }

        private ContentDocument CreateOrShowScript(UnloadedRoom selectedRoom)
        {
            ContentDocument scriptEditor = GetScriptEditor(selectedRoom);
            ContentDocument document = _roomScriptEditors[selectedRoom.Number];
            document.TreeNodeID = TREE_PREFIX_ROOM_SCRIPT + selectedRoom.Number;
            _guiController.AddOrShowPane(document);
            return scriptEditor;
        }

        private DockData GetPreviousDockData()
        {
            if (_roomSettings != null && _roomSettings.Control != null && 
                _roomSettings.Control.DockingContainer != null &&
                _roomSettings.Control.DockingContainer.DockState != DockingState.Hidden &&
                _roomSettings.Control.DockingContainer.DockState != DockingState.Unknown)
            {
                EditorContentPanel control = _roomSettings.Control;
                if (control.DockingContainer.FloatPane == null ||
                    control.DockingContainer.FloatPane.FloatWindow == null)
                {
                    return new DockData(control.DockingContainer.DockState, Rectangle.Empty);
                }
                IFloatWindow floatWindow = control.DockingContainer.FloatPane.FloatWindow;
                return new DockData(control.DockingContainer.DockState, new Rectangle(
                    floatWindow.Location, floatWindow.Size));
            }
            return null;
        }

        private void LoadRoom(string controlID)
        {
            DockData previousDockData = GetPreviousDockData();            
            UnloadedRoom selectedRoom = FindRoomByID(Convert.ToInt32(controlID.Substring(3)));
            if ((_loadedRoom == null) || (_roomSettings == null) ||
                (selectedRoom.Number != _loadedRoom.Number))
            {
                LoadDifferentRoom(selectedRoom);
            }
            if (_roomSettings != null)
            {
                if (_roomSettings.Control.IsDisposed)
                {
                    CreateRoomSettings(previousDockData);
                }
                _roomSettings.TreeNodeID = controlID;
                _guiController.AddOrShowPane(_roomSettings);
            }
        }

        protected override ContentDocument GetDocument(ScriptEditor editor)
        {
            foreach (ContentDocument doc in _roomScriptEditors.Values)
            {
                if (doc.Control == editor)
                {
                    return doc;
                }
            }
            return null;
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
            _fileWatchers.Enabled = false;

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

            _loadedRoom = new Room(LoadData(newRoom)) { Script = newRoom.Script };
            LoadImageCache();
            _fileWatchers.Clear();
            _fileWatchers.AddRange(LoadFileWatchers());

            // TODO: group these in some UpdateRoomToNewVersion method
            _loadedRoom.Modified |= ImportExport.CreateInteractionScripts(_loadedRoom, errors);
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

        private bool ApplyDefaultMaskResolution(Room room)
        {
            int mask = _agsEditor.CurrentGame.Settings.DefaultRoomMaskResolution;
            if (mask != room.MaskResolution)
            {
                ((IRoomController)this).AdjustMaskResolution(mask);
                return true;
            }
            return false;
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
                DockData previousDockData = GetPreviousDockData();
                UnloadCurrentRoomAndGreyOutTree();

                if (!Directory.Exists(newRoom.Directory))
                {
                    _guiController.ShowMessage($"The room directory \"{newRoom.Directory}\" could not be found. Unable to open this room.", MessageBoxIcon.Error);
                }
                else if (!File.Exists(newRoom.DataFileName))
                {
                    _guiController.ShowMessage($"The room data file \"{newRoom.DataFileName} could not be found. Unable to open this room.", MessageBoxIcon.Error);
                }
				else
				{
					CompileMessages errors = new CompileMessages();

					LoadNewRoomIntoMemory(newRoom, errors);

					_loadedRoom.RoomModifiedChanged += _modifiedChangedHandler;

                    CreateRoomSettings(previousDockData);

					if (_loadedRoom.Modified)
					{
						CreateOrShowScript(_loadedRoom);
					}

                    ContentDocument document;
					if (_roomScriptEditors.TryGetValue(_loadedRoom.Number, out document))
					{
                        ScriptEditor editor = ((ScriptEditor)document.Control);
                        editor.Room = _loadedRoom;
						if (_loadedRoom.Modified)
						{
                            editor.ScriptModifiedExternally();
						}
					}

                    ProjectTree treeController = _guiController.ProjectTree;
					treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_NODE + _loadedRoom.Number, ROOM_ICON_LOADED);
					treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_SETTINGS + _loadedRoom.Number, ROOM_ICON_LOADED);

					_guiController.ShowOutputPanel(errors);
                    return true;
                }

                return false;
            }
        }

        private void CreateRoomSettings(DockData previousDockData)
        {
            string paneTitle = "Room " + _loadedRoom.Number + (_loadedRoom.Modified ? " *" : "");

            RoomSettingsEditor editor = new RoomSettingsEditor(_loadedRoom, this);
            _roomSettings = new ContentDocument(editor,
                paneTitle, this, ROOM_ICON_LOADED, ConstructPropertyObjectList(_loadedRoom));
            if (previousDockData != null && previousDockData.DockState != DockingState.Document)
            {
                _roomSettings.PreferredDockData = previousDockData;
            }
            _roomSettings.SelectedPropertyGridObject = _loadedRoom;
            editor.SaveRoom += new RoomSettingsEditor.SaveRoomHandler(RoomEditor_SaveRoom);
            editor.AbandonChanges += new RoomSettingsEditor.AbandonChangesHandler(RoomsComponent_AbandonChanges);
            RoomDesignData.LoadFromUserFile(_loadedRoom, editor);
            editor.RefreshLayersTree();
            // Reset the Modified flag in case initialization triggered some events
            editor.DesignModified = false;
        }

        private void RoomsComponent_AbandonChanges(Room room)
        {
            if (room == _loadedRoom)
            {
                UnloadCurrentRoomAndGreyOutTree();
            }
        }

        private bool RoomEditor_SaveRoom(Room room, RoomSettingsEditor editor)
        {
            return SaveRoomAlwaysAndShowErrors(room, editor);
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
                RoomListTypeConverter.RefreshRoomList();
            }

			if ((propertyName == UnloadedRoom.PROPERTY_NAME_NUMBER) && (_loadedRoom != null))
			{
				int numberRequested = _loadedRoom.Number;
				_loadedRoom.Number = Convert.ToInt32(oldValue);
                RenameRoom(_loadedRoom.Number, numberRequested);
                RoomListTypeConverter.RefreshRoomList();
            }

            if ((propertyName == Room.PROPERTY_NAME_MASKRESOLUTION) && (_loadedRoom != null))
            {
                AdjustRoomMaskResolution(Convert.ToInt32(oldValue), _loadedRoom.MaskResolution);
            }

            // TODO: wish we could forward event to the CharacterComponent.OnPropertyChanged,
            // but its implementation relies on it being active Pane!
            if ((_guiController.ActivePane.SelectedPropertyGridObject is Character) &&
                (propertyName == Character.PROPERTY_NAME_SCRIPTNAME))
            {
                Character character = (Character)_guiController.ActivePane.SelectedPropertyGridObject;
                character.ScriptName = (string)oldValue;
                _guiController.ShowMessage("You cannot edit a character's script name from here. Open the Character Editor for the character then try again.", MessageBoxIcon.Information);
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
			else if (SaveRoomAlwaysAndShowErrors(_loadedRoom, _roomSettings?.Control as RoomSettingsEditor))
			{
				UnloadedRoom oldRoom = FindRoomByID(currentNumber);
				UnloadedRoom tempNewRoom = new UnloadedRoom(numberRequested);
				CloseRoomScriptEditorIfOpen(currentNumber);
				UnloadCurrentRoom();

				_agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldRoom.FileName, tempNewRoom.FileName);
                _agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldRoom.UserFileName, tempNewRoom.UserFileName);
                _agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldRoom.ScriptFileName, tempNewRoom.ScriptFileName);
                _agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldRoom.DataFileName, tempNewRoom.DataFileName);

                for (int i = 0; i < Room.MAX_BACKGROUNDS; i++)
                {
                    string oldBackgroundFileName = oldRoom.GetBackgroundFileName(i);
                    if (File.Exists(oldBackgroundFileName))
                        _agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldBackgroundFileName, tempNewRoom.GetBackgroundFileName(i));
                }

                foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)).Cast<RoomAreaMaskType>().Where(t => t != RoomAreaMaskType.None))
                {
                    string oldMaskFileName = oldRoom.GetMaskFileName(mask);
                    if (File.Exists(oldMaskFileName))
                        _agsEditor.SourceControlProvider.RenameFileOnDiskAndInSourceControl(oldMaskFileName, tempNewRoom.GetMaskFileName(mask));
                }

				oldRoom.Number = numberRequested;
                XDocument newRoomXml = XDocument.Load(tempNewRoom.DataFileName);
                newRoomXml.Element("Room").SetElementValue("Number", numberRequested);
                using (var writer = new XmlTextWriter(tempNewRoom.DataFileName, Types.Utilities.UTF8))
                {
                    newRoomXml.Save(writer);
                }

				LoadDifferentRoom(oldRoom);
                _roomSettings.TreeNodeID = TREE_PREFIX_ROOM_SETTINGS + numberRequested;
				_guiController.AddOrShowPane(_roomSettings);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
				RePopulateTreeView();
			}
		}

        /// <summary>
        /// Resize room masks to match current MaskResolution property.
        /// </summary>
        private void AdjustRoomMaskResolution(int oldValue, int newValue)
        {
            if (newValue > oldValue) // this is a divisor
            {
                if (Factory.GUIController.ShowQuestion("The new mask resolution is smaller and this will reduce mask's precision and some pixels may be lost in the process. Do you want to proceed?") != DialogResult.Yes)
                    return;
            }
            ((IRoomController)this).AdjustMaskResolution(newValue);
        }

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New room...", null));
            menu.Add(new MenuCommand(COMMAND_IMPORT_ROOM, "Import existing room...", null));
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(MenuCommand.Separator);
            menu.Add(new MenuCommand(COMMAND_SORT_BY_NUMBER, "Sort rooms by number", null));
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            if ((controlID.StartsWith(TREE_PREFIX_ROOM_SETTINGS)) ||
                     (controlID.StartsWith(TREE_PREFIX_ROOM_SCRIPT)))
            {
                // No right-click menu for these
            }
            else if (controlID.StartsWith(TREE_PREFIX_ROOM_NODE))
            {
                _rightClickedRoomNumber = Convert.ToInt32(controlID.Substring(3));

                menu.Add(new MenuCommand(COMMAND_LOAD_ROOM, "Edit this room", null));
                menu.Add(MenuCommand.Separator);

                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this room", null));                
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
            // Allow room mask resolutions from 1:1 to 1:4
            RoomMaskResolutionTypeConverter.SetResolutionRange(1, 4);
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
                ContentDocument document;
                if (_roomScriptEditors.TryGetValue(_loadedRoom.Number, out document))
                {
                    ((ScriptEditor)document.Control).UpdateScriptObjectWithLatestTextInWindow();
                }
                script = _loadedRoom.Script;
            }
        }

        private void GUIController_OnScriptChanged(Script script)
        {
            ContentDocument document;
            if ((_loadedRoom != null) &&
                (script == _loadedRoom.Script) &&
                (_roomScriptEditors.TryGetValue(_loadedRoom.Number, out document)))
            {
                ((ScriptEditor)document.Control).ScriptModifiedExternally();
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
			foreach (ScriptAndHeader script in _agsEditor.CurrentGame.RootScriptFolder.AllItemsFlat)
			{
				if ((Utilities.DoesFileNeedRecompile(script.Header.FileName, roomFileName)))
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
            foreach (UnloadedRoom unloadedRoom in _agsEditor.CurrentGame.RootRoomFolder.AllItemsFlat)
            {
                IEnumerable<string> missingMasks = Enum
                    .GetValues(typeof(RoomAreaMaskType))
                    .Cast<RoomAreaMaskType>()
                    .Where(m => m != RoomAreaMaskType.None)
                    .Select(m => unloadedRoom.GetMaskFileName(m))
                    .Where(f => !File.Exists(f));

                if (!File.Exists(unloadedRoom.ScriptFileName))
                {
                    errors.Add(new CompileError("File not found: " + unloadedRoom.ScriptFileName + "; If you deleted this file, use the Exclude From Game option to remove it from the game."));
                    success = false;
                }
                else if (!File.Exists(unloadedRoom.DataFileName))
                {
                    errors.Add(new CompileError("File not found: " + unloadedRoom.DataFileName + "; If you deleted this file, use the Exclude From Game option to remove it from the game."));
                    success = false;
                }
                else if (!File.Exists(unloadedRoom.GetBackgroundFileName(0)))
                {
                    errors.Add(new CompileError("File not found: " + unloadedRoom.GetBackgroundFileName(0) + "; If you deleted this file, use the Exclude From Game option to remove it from the game."));
                    success = false;
                }
                else if (missingMasks.Any())
                {
                    errors.AddRange(missingMasks.Select(f => new CompileError("File not found: " + f + "; If you deleted this file, use the Exclude From Game option to remove it from the game.")));
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
					UnloadCurrentRoomAndGreyOutTree();
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
            if (_loadedRoom != null)
            {
                evArgs.AllowCompilation = SaveRoomIfModifiedAndShowErrors(_loadedRoom, _roomSettings?.Control as RoomSettingsEditor);
            }

            if (evArgs.AllowCompilation)
            {
				evArgs.AllowCompilation = RecompileAnyRoomsWhereTheScriptHasChanged(evArgs.Errors, evArgs.ForceRebuild);
            }
        }

        private void AGSEditor_PreSaveGame(PreSaveGameEventArgs evArgs)
        {
            if (_loadedRoom != null)
            {
				if (!SaveRoomIfModifiedAndShowErrors(_loadedRoom, _roomSettings?.Control as RoomSettingsEditor))
				{
					evArgs.SaveSucceeded = false;
				}
            }
        }

		private bool CheckOutAllRoomsAndScripts()
		{
			List<string> roomFileNamesToRebuild = new List<string>();
            foreach (UnloadedRoom unloadedRoom in _agsEditor.CurrentGame.RootRoomFolder.AllItemsFlat)
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

            foreach (UnloadedRoom unloadedRoom in _agsEditor.CurrentGame.RootRoomFolder.AllItemsFlat)
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

                TextProcessingHelper.ProcessProperties(processor, _agsEditor.CurrentGame.PropertySchema, room.Properties, errors);

                foreach (RoomHotspot hotspot in room.Hotspots)
                {
                    hotspot.Description = processor.ProcessText(hotspot.Description, GameTextType.ItemDescription);
                    TextProcessingHelper.ProcessProperties(processor, _agsEditor.CurrentGame.PropertySchema, hotspot.Properties, errors);
                }

                foreach (RoomObject obj in room.Objects)
                {
                    obj.Description = processor.ProcessText(obj.Description, GameTextType.ItemDescription);
                    TextProcessingHelper.ProcessProperties(processor, _agsEditor.CurrentGame.PropertySchema, obj.Properties, errors);
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

        protected override ProjectTreeItem CreateTreeItemForItem(IRoom room)
		{
			string iconName = ROOM_ICON_UNLOADED;

			if ((_loadedRoom != null) && (room.Number == _loadedRoom.Number))
			{
				iconName = ROOM_ICON_LOADED;
			}

			ProjectTree treeController = _guiController.ProjectTree;
			ProjectTreeItem treeItem = treeController.AddTreeBranch(this, TREE_PREFIX_ROOM_NODE + room.Number, room.Number.ToString() + ": " + room.Description, iconName);
			treeController.AddTreeLeaf(this, TREE_PREFIX_ROOM_SETTINGS + room.Number, "Edit room", iconName);
            treeController.AddTreeLeaf(this, TREE_PREFIX_ROOM_SCRIPT + room.Number, "Room script", SCRIPT_ICON);
            return treeItem;
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

        void IRoomController.Save()
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            _fileWatchers.TemporarilyDisable(() =>
            {
                IsBeingSaved = true;
                SaveImages();
                using (var writer = new XmlTextWriter(_loadedRoom.DataFileName, Types.Utilities.UTF8))
                {
                    _loadedRoom.ToXmlDocument().Save(writer);
                }
                IsBeingSaved = false;
                LastSavedAt = DateTime.Now;
                _loadedRoom.Modified = false;
            });

            SaveCrm();
        }

        Bitmap IRoomController.GetBackground(int background)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            return _backgroundCache[background].Clone() as Bitmap;
        }

        void IRoomController.SetBackground(int background, Bitmap bmp)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }
            if (bmp == null)
            {
                throw new ArgumentNullException(nameof(bmp));
            }

            Bitmap newBmp = new Bitmap(bmp);
            bool hasResolutionChanged = _loadedRoom.Width != newBmp.Width || _loadedRoom.Height != newBmp.Height;
            _loadedRoom.Width = newBmp.Width;
            _loadedRoom.Height = newBmp.Height;
            _loadedRoom.ColorDepth = newBmp.GetColorDepth();

            if (_loadedRoom.ColorDepth == 8)
            {
                ColorPalette palette = newBmp.Palette;
                foreach (var color in _agsEditor.CurrentGame.Palette.Where(p => p.ColourType == PaletteColourType.Locked))
                {
                    palette.Entries[color.Index] = color.Colour;
                }
                newBmp.Palette = palette;

                newBmp.SetGlobalPaletteFromPalette();
                // TODO Implement remap_background from AGS.Native
            }

            if (background >= _backgroundCache.Count)
            {
                _backgroundCache.Add(newBmp);
                _fileWatchers[_loadedRoom.GetBackgroundFileName(_loadedRoom.BackgroundCount++)].Enabled = true;
            }
            else
            {
                _backgroundCache[background]?.Dispose();
                _backgroundCache[background] = newBmp;
            }

            // If size or resolution has changed, reset masks
            if (hasResolutionChanged)
            {
                ResetMaskCache();
            }

            _loadedRoom.Modified = true;
        }

        void IRoomController.DeleteBackground(int background)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            _backgroundCache[background]?.Dispose();
            _backgroundCache.RemoveAt(background);
            _fileWatchers[_loadedRoom.GetBackgroundFileName(--_loadedRoom.BackgroundCount)].Enabled = false;
            _loadedRoom.Modified = true;
        }

        Bitmap IRoomController.GetMask(RoomAreaMaskType mask)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            return _maskCache[mask].Clone() as Bitmap;
        }

        void IRoomController.SetMask(RoomAreaMaskType mask, Bitmap bmp)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }
            if (bmp == null)
            {
                throw new ArgumentNullException(nameof(bmp));
            }

            if (ValidateMask(mask, bmp))
            {
                Bitmap toDispose;
                _maskCache.TryGetValue(mask, out toDispose);
                toDispose?.Dispose();
                _maskCache[mask] = bmp.Clone() as Bitmap;
                _loadedRoom.Modified = true;
            }
        }

        int IRoomController.GetAreaMaskPixel(RoomAreaMaskType maskType, int x, int y)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            // adjust coordinates to mask resolution
            double scale = _loadedRoom.GetMaskScale(maskType);
            x = (int)(x * scale);
            y = (int)(y * scale);

            Bitmap mask = _maskCache[maskType];

            if (x < 0 || y < 0 || x > mask.Width || y > mask.Height) return 0;

            return mask.GetRawData()[(y * mask.Width) + x];
        }

        void IRoomController.DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, int scaleFactor)
		{
			((IRoomController)this).DrawRoomBackground(g, new Point(x, y), backgroundNumber, (double)scaleFactor, RoomAreaMaskType.None, 0, 0);
		}

		void IRoomController.DrawRoomBackground(Graphics g, int x, int y, int backgroundNumber, int scaleFactor, RoomAreaMaskType maskType, int maskTransparency, int selectedArea)
		{
            ((IRoomController)this).DrawRoomBackground(g, new Point(x, y), backgroundNumber, (double)scaleFactor, maskType, maskTransparency, selectedArea);
        }

        void IRoomController.DrawRoomBackground(Graphics g, Point point, int backgroundNumber, double scaleFactor, RoomAreaMaskType maskType, int maskTransparency, int selectedArea)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }
            if ((maskTransparency < 0) || (maskTransparency > 100))
            {
                throw new ArgumentOutOfRangeException("maskTransparency", "Mask Transparency must be between 0 and 100");
            }

            Bitmap background = _backgroundCache[backgroundNumber];

            // x and y is already scaled from outside the method call
            Rectangle drawingArea = new Rectangle(
                point.X, point.Y, (int)(background.Width * scaleFactor), (int)(background.Height * scaleFactor));

            g.DrawImage(background, drawingArea);

            if (maskType != RoomAreaMaskType.None)
            {
                Bitmap mask8bpp = _maskCache[maskType];
                ColorPalette paletteBackup = mask8bpp.Palette;
                ColorPalette paletteDrawing = mask8bpp.Palette;

                // Color not selected mask areas to gray
                if (_grayOutMasks)
                {
                    for (int i = 1; i < 256; i++) // Skip i == 0 because no area should be transparent
                    {
                        const int intensity = 6; // Force the gray scale lighter so that it's easier to see
                        int gray = i < Room.MAX_HOTSPOTS && i > 0 ? ((Room.MAX_HOTSPOTS - i) % 30) * intensity : 0;
                        paletteDrawing.Entries[i] = Color.FromArgb(gray, gray, gray);
                    }

                    // Highlight the currently selected area colour
                    if (selectedArea > 0)
                    {
                        // if a bright colour, use it, else, draw in red
                        paletteDrawing.Entries[selectedArea] = selectedArea < 15 && selectedArea != 7 && selectedArea != 8
                            ? _agsEditor.CurrentGame.Palette[selectedArea].Colour
                            : Color.FromArgb(60, 0, 0);
                    }
                }

                // Prepare alpha component
                float alpha = (100 - maskTransparency + 155) / 255f;
                ColorMatrix colorMatrix = new ColorMatrix { Matrix33 = alpha };
                ImageAttributes attributes = new ImageAttributes();
                attributes.SetColorMatrix(colorMatrix, ColorMatrixFlag.Default, ColorAdjustType.Bitmap);

                // Force that palette index 0 (No Area) is transparent
                paletteDrawing.Entries[0] = Color.FromArgb(0, 255, 255, 255);

                mask8bpp.Palette = paletteDrawing;
                g.DrawImage(mask8bpp, drawingArea, 0, 0, mask8bpp.Width, mask8bpp.Height, GraphicsUnit.Pixel, attributes);
                mask8bpp.Palette = paletteBackup;
            }
        }

        void IRoomController.AdjustMaskResolution(int maskResolution)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            _loadedRoom.MaskResolution = maskResolution;
            Bitmap bg = _backgroundCache[0];
            int baseWidth = bg.Width;
            int baseHeight = bg.Height;

            foreach (RoomAreaMaskType maskType in Enum.GetValues(typeof(RoomAreaMaskType)))
            {
                if (maskType == RoomAreaMaskType.None) continue;

                using (Bitmap mask = _maskCache[maskType])
                {
                    double scale = _loadedRoom.GetMaskScale(maskType);
                    _maskCache[maskType] = mask.ScaleIndexed((int)(baseWidth * scale), (int)(baseHeight * scale));
                }
            }

            _loadedRoom.Modified = true;
        }

		bool IRoomController.GreyOutNonSelectedMasks
		{
            set { _grayOutMasks = value; }
        }

        protected override bool CanFolderBeDeleted(UnloadedRoomFolder folder)
        {
            foreach (UnloadedRoom room in folder.AllItemsFlat)
            {
                if (!EnsureNoCharactersStartInRoom(room.Number)) return false;
            }
            return true;
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its rooms?" + Environment.NewLine + Environment.NewLine + "If any of the rooms are referenced in code by their number it could cause crashes in the game.";
        }

        protected override UnloadedRoomFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootRoomFolder;
        }

        protected override IList<IRoom> GetFlatList()
        {
            return null;
        }

        private void LoadImageCache()
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            ClearImageCache();

            for (int i = 0; i < _loadedRoom.BackgroundCount; i++)
            {
                if (File.Exists(_loadedRoom.GetBackgroundFileName(i)))
                {
                    _backgroundCache.Add(LoadBackground(i));
                }
            }

            bool imageNotFound = false;

            if (!_backgroundCache.Any())
            {
                _backgroundCache.Add(new Bitmap(_loadedRoom.Width, _loadedRoom.Height));
                _loadedRoom.BackgroundCount = 1;
                imageNotFound = true;
                _guiController.ShowMessage(
                    $"Could not to find any background images at \"{_loadedRoom.Directory}\", an empty " +
                    $"default image will be used instead.",
                    MessageBoxIcon.Warning);
            }
            else if (_loadedRoom.BackgroundCount != _backgroundCache.Count)
            {
                _loadedRoom.BackgroundCount = _backgroundCache.Count;
                imageNotFound = true;
            }
            
            _loadedRoom.ColorDepth = _backgroundCache[0].GetColorDepth();

            foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
            {
                if (mask == RoomAreaMaskType.None)
                {
                    continue;
                }

                if (File.Exists(_loadedRoom.GetMaskFileName(mask)))
                {
                    ((IRoomController)this).SetMask(mask, LoadMask(mask));
                }
                else
                {
                    _maskCache[mask] = CreateMaskBitmap(mask, _loadedRoom.Width, _loadedRoom.Height);
                    imageNotFound = true;
                    _guiController.ShowMessage(
                        $"Could not to find mask at \"{_loadedRoom.GetMaskFileName(mask)}\", an empty " +
                        $"default image will be used instead.",
                        MessageBoxIcon.Warning);
                }
            }

            _loadedRoom.Modified = imageNotFound;
        }

        private XmlNode LoadData(UnloadedRoom room)
        {
            XmlDocument xml = new XmlDocument();

            using (FileStream filestream = File.Open(room.DataFileName, FileMode.Open, FileAccess.Read, FileShare.ReadWrite))
            using (BinaryReader reader = new BinaryReader(filestream))
            {
                byte[] bytes = reader.ReadBytes((int)reader.BaseStream.Length);
                string xmlContent = Encoding.Default.GetString(bytes);
                xml.LoadXml(xmlContent);
                return xml.SelectSingleNode("Room");
            }
        }

        private void RefreshData()
        {
            SerializeUtils.DeserializeFromXML(_loadedRoom, LoadData(_loadedRoom));
            _guiController.RefreshPropertyGrid();
        }

        private Bitmap LoadBackground(int i) => LoadNonLockedBitmap(_loadedRoom.GetBackgroundFileName(i));

        private void RefreshBackground(int i)
        {
            _backgroundCache[i]?.Dispose();
            _backgroundCache[i] = LoadBackground(i);
            ((RoomSettingsEditor)_roomSettings.Control).InvalidateDrawingBuffer();
        }

        /// <summary>
        /// Creates mask bitmap for the given mask type of the requested size.
        /// </summary>
        private Bitmap CreateMaskBitmap(RoomAreaMaskType mask, int width, int height)
        {
            double scale = _loadedRoom.GetMaskScale(mask);
            var bitmap = new Bitmap((int)(width * scale), (int)(height * scale), PixelFormat.Format8bppIndexed);
            bitmap.SetPaletteFromGlobalPalette();
            return bitmap;
        }

        private Bitmap LoadMask(RoomAreaMaskType mask) => LoadNonLockedBitmap(_loadedRoom.GetMaskFileName(mask));

        private void RefreshMask(RoomAreaMaskType mask)
        {
            _maskCache[mask]?.Dispose();
            _maskCache[mask] = LoadMask(mask);
            ((RoomSettingsEditor)_roomSettings.Control).InvalidateDrawingBuffer();
        }

        /// <summary>
        /// Validates if the bitmap is valid to be used as a mask, or if possible, fixes it so
        /// that it can be usable. (For example by replacing illgal pixels with legal pixels)
        /// </summary>
        private bool ValidateMask(RoomAreaMaskType type, Bitmap newMask)
        {
            if (type == RoomAreaMaskType.None) throw new ArgumentException("Don't use mask type None.");
            if (newMask == null) throw new NullReferenceException($"{nameof(newMask)} is null.");
            if (newMask.GetColorDepth() != 8)
            {
                _guiController.ShowMessage($"Trying to set an invalid {type} mask, make sure it's an 8-bit image.", MessageBoxIcon.Warning);
                return false;
            }

            int maxColor = Room.GetMaskMaxColor(type);
            bool invalidPixel = false;

            newMask.SetRawData(newMask.GetRawData().Select(p =>
            {
                if (p >= maxColor)
                {
                    invalidPixel = true;
                    return (byte)0;
                }
                return p;
            }).ToArray());

            if (invalidPixel)
            {
                _guiController.ShowMessage(
                    $"Invalid colours were found in the {type} mask. They have now been removed." +
                    "\n\nWhen drawing a mask in an external paint package, you need to make" +
                    "sure that the image is set as 256-colour (Indexed Palette), and that" +
                    "you use the first 16 colours in the palette for drawing your areas. Palette " +
                    "entry 0 corresponds to No Area, palette index 1 corresponds to area 1, and " +
                    "so forth.",
                    MessageBoxIcon.Information);
            }

            return true;
        }

        private Bitmap LoadNonLockedBitmap(string fileName)
        {
            using (MemoryStream ms = new MemoryStream(File.ReadAllBytes(fileName)))
            {
                return new Bitmap(ms);
            }
        }

        private void ClearImageCache()
        {
            ClearBackgroundCache();
            ClearMaskCache();
        }

        private void ClearBackgroundCache()
        {
            for (int i = 0; i < _backgroundCache.Count; i++)
            {
                _backgroundCache[i]?.Dispose();
            }
            _backgroundCache.Clear();
        }

        private void ClearMaskCache()
        {
            foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
            {
                if (mask == RoomAreaMaskType.None)
                    continue;

                if (_maskCache.ContainsKey(mask))
                {
                    _maskCache[mask]?.Dispose();
                    _maskCache[mask] = null;
                }
            }
        }

        private void ResetMaskCache()
        {
            foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
            {
                if (mask == RoomAreaMaskType.None)
                    continue;

                if (_maskCache.ContainsKey(mask))
                {
                    _maskCache[mask]?.Dispose();
                    _maskCache[mask] = CreateMaskBitmap(mask, _loadedRoom.Width, _loadedRoom.Height);
                }
            }
        }

        private void SaveImages()
        {
            lock (_loadedRoom)
            {
                for (int i = 0; i < Room.MAX_BACKGROUNDS; i++)
                {
                    string fileName = _loadedRoom.GetBackgroundFileName(i);

                    if (i < _backgroundCache.Count)
                        _backgroundCache[i].Save(fileName, ImageFormat.Png);
                    else
                        File.Delete(fileName);
                }

                foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
                {
                    if (mask == RoomAreaMaskType.None)
                        continue;

                    string fileName = _loadedRoom.GetMaskFileName(mask);
                    _maskCache[mask].Save(fileName, ImageFormat.Png);
                }
            }
        }

        private IEnumerable<FileWatcher> LoadFileWatchers()
        {
            yield return new FileWatcher(_loadedRoom.DataFileName, this, RefreshData);

            for (int i = 0; i < Room.MAX_BACKGROUNDS; i++)
            {
                // Have to make a copy otherwise i will be equal to Room.MAX_BACKGROUNDS when loadFile callback is executed
                int roomNumber = i; 
                yield return new FileWatcher(_loadedRoom.GetBackgroundFileName(roomNumber), this, () => RefreshBackground(roomNumber))
                {
                    Enabled = roomNumber < _loadedRoom.BackgroundCount
                };
            }

            foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)).Cast<RoomAreaMaskType>().Where(m => m != RoomAreaMaskType.None))
            {
                yield return new FileWatcher(_loadedRoom.GetMaskFileName(mask), this, () => RefreshMask(mask));
            }
        }

        #region Upgrade Crm Format To Open Format
        /// <summary>
        /// Upgrades the room format from .crm to open format with text files and images directly accessible from disk
        /// </summary>
        /// <remarks>
        /// This easily maxes the work capacity of a single thread and runs for a while so this is a good candidate
        /// for parallel execution. However the native proxy is designed around the <see cref="Room._roomStructPtr"/>
        /// which can only hold the value of single room at a time so thread execution would crash. It might not be
        /// worthwhile to refactor this hindrance if we're getting a standalone room CLI tool in the future that can
        /// do the same job.
        /// </remarks>
        private async void ConvertAllRoomsFromCrmToOpenFormat()
        {
            if (_agsEditor.CurrentGame.SavedXmlVersionIndex >= AGSEditor.AGS_4_0_0_XML_VERSION_INDEX)
                return; // Upgrade already completed

            IList<IRoom> rooms = _agsEditor.CurrentGame.Rooms;

            // If the room directory we want to write to already exists then backup
            if (Directory.Exists(UnloadedRoom.ROOM_DIRECTORY))
            {
                string backupRootDir = Enumerable
                    .Range(0, int.MaxValue)
                    .Select(i => $"{UnloadedRoom.ROOM_DIRECTORY}Backup-{i}")
                    .First(dir => !Directory.Exists(dir));

                DirectoryInfo roomDir = new DirectoryInfo(UnloadedRoom.ROOM_DIRECTORY);
                DirectoryInfo backupDir = new DirectoryInfo(backupRootDir);
                roomDir.CopyAll(backupDir);
                // Don't crash the upgrade if a file can't be deleted
                roomDir.DeleteWithoutException(recursive: true);
            }

            // Now upgrade
            object progressLock = new object();
            string progressText = "Converting rooms from .crm to open format.";
            using (Progress progressForm = new Progress(rooms.Count, progressText))
            {
                progressForm.Show();
                int progress = 0;
                Action progressReporter = () =>
                {
                    lock (progressLock) { progress++; }
                    progressForm.SetProgress(progress, $"{progressText} {progress} of {rooms.Count} rooms converted.");
                };

                var roomsConvertingTasks = rooms
                    .Cast<UnloadedRoom>()
                    .SelectMany(r => ConvertRoomFromCrmToOpenFormat(r, progressReporter))
                    .ToArray();
                await Task.WhenAll(roomsConvertingTasks);
            }
        }

        /// <summary>
        /// Converts a single room from .crm to open format.
        /// </summary>
        /// <param name="room">The room to convert to open format</param>
        /// <returns>A collection of tasks that converts the room async.</returns>
        private IEnumerable<Task> ConvertRoomFromCrmToOpenFormat(UnloadedRoom unloadedRoom, Action report = null, bool isNewRoom=false)
        {
            Room room = _nativeProxy.LoadRoom(unloadedRoom);

            for (int i = 0; i < room.BackgroundCount; i++)
                yield return SaveAndDisposeBitmapAsync(_nativeProxy.GetBitmapForBackground(room, i), room.GetBackgroundFileName(i));

            yield return SaveXmlAsync(room.ToXmlDocument(), room.DataFileName);

            foreach (RoomAreaMaskType type in Enum.GetValues(typeof(RoomAreaMaskType)).Cast<RoomAreaMaskType>().Where(m => m != RoomAreaMaskType.None))
                yield return SaveAndDisposeBitmapAsync(_nativeProxy.ExportAreaMask(room, type), room.GetMaskFileName(type));

            if (!isNewRoom)
            {
                string oldScriptFileName = $"room{room.Number}.asc";
                if (File.Exists(oldScriptFileName))
                    File.Move(oldScriptFileName, room.ScriptFileName);

                string oldUserFileName = $"room{room.Number}.crm.user";
                if (File.Exists(oldUserFileName))
                    File.Move(oldUserFileName, room.UserFileName);
            }

            report?.Invoke();
        }

        private Task SaveXmlAsync(XmlDocument document, string filename) => Task.Run(() =>
        {
            using (var writer = new XmlTextWriter(filename, Types.Utilities.UTF8))
            {
                document.Save(writer);
            }
        });

        private Task SaveAndDisposeBitmapAsync(Bitmap bmp, string filename) => Task.Run(() =>
        {
            using (bmp)
            {
                bmp.Save(filename, ImageFormat.Png);
            }
        });

        /// <summary>
        /// Saves .crm file from open format
        /// </summary>
        private void SaveCrm()
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            var masks = new List<Bitmap>();
            masks.AddRange(_maskCache.OrderBy(v => v.Key).Select(v => v.Value));
            _nativeProxy.SaveRoom(_loadedRoom, _backgroundCache,
                _agsEditor.Settings.RemapPalettizedBackgrounds, sharePalette: false, masks: masks);
        }
        #endregion
    }
}
