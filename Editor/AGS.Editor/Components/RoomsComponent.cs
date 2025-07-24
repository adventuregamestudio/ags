using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;
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
        private const string COMMAND_CHANGE_NUMBER = "ChangeRoomID";

        private const string ROOM_ICON = "RoomsIcon";
        private const string ROOM_ICON_UNLOADED = "RoomIcon";
        private const string ROOM_ICON_LOADED = "RoomColourIcon";
        private const string ROOM_ICON_SAVE_UNLOADED = "RoomIconSaving";
        private const string ROOM_ICON_SAVE_LOADED = "RoomColourIconSaving";
        private const string SCRIPT_ICON = "ScriptIcon";

        /// <summary>
        /// RoomImage contains one of the room's images (bg or mask)
        /// paired with its Modified state.
        /// </summary>
        private class RoomImage : IDisposable
        {
            public Bitmap Image;
            public bool Modified;

            public RoomImage(Bitmap image, bool modified)
            {
                Image = image;
                Modified = modified;
            }

            public void Dispose()
            {
                Image?.Dispose();
            }
        }

        private readonly List<RoomImage> _backgroundCache = new List<RoomImage>(Room.MAX_BACKGROUNDS);
        private readonly Dictionary<RoomAreaMaskType, RoomImage> _maskCache = new Dictionary<RoomAreaMaskType, RoomImage>(Enum.GetValues(typeof(RoomAreaMaskType)).Length);
        private readonly FileWatcherCollection _fileWatchers = new FileWatcherCollection();

        public event PreSaveRoomHandler PreSaveRoom;
        private ContentDocument _roomSettings;
        private Dictionary<int,ContentDocument> _roomScriptEditors = new Dictionary<int,ContentDocument>();
        private Room _loadedRoom;
        // Current room palette combines game-wide slots from the default game pal,
        // and background slots from the room background pal
        private PaletteEntry[] _roomPalette;
        private NativeProxy _nativeProxy;
        private int _rightClickedRoomNumber;
        private Room.RoomModifiedChangedHandler _modifiedChangedHandler;
		private object _roomLoadingOrSavingLock = new object();
        private bool _grayOutMasks = true;

        public RoomsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, ROOMS_COMMAND_ID)
        {
            _guiController.RegisterIcon(ROOM_ICON, Resources.ResourceManager.GetIcon("room.ico"));
            _guiController.RegisterIcon(ROOM_ICON_UNLOADED, Resources.ResourceManager.GetIcon("room-item.ico"));
            _guiController.RegisterIcon(ROOM_ICON_LOADED, Resources.ResourceManager.GetIcon("roomsareas.ico"));
            _guiController.RegisterIcon("MenuIconSaveRoom", Resources.ResourceManager.GetIcon("menu_file_save-room.ico"));
            _guiController.RegisterIcon(ROOM_ICON_SAVE_UNLOADED, Resources.ResourceManager.GetIcon("room-item-save.ico"));
            _guiController.RegisterIcon(ROOM_ICON_SAVE_LOADED, Resources.ResourceManager.GetIcon("menu_file_save-room.over.ico"));

            MenuCommands commands = new MenuCommands(GUIController.FILE_MENU_ID, 50);
            commands.Commands.Add(new MenuCommand(COMMAND_SAVE_ROOM, "Save Room", Keys.Control | Keys.R, "MenuIconSaveRoom"));
            _guiController.AddMenuItems(this, commands);

			_nativeProxy = Factory.NativeProxy;
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Rooms", "RoomsIcon");
            _guiController.OnZoomToFile += GUIController_OnZoomToFile;
            _guiController.OnGetScript += GUIController_OnGetScript;
            _guiController.OnScriptChanged += GUIController_OnScriptChanged;
            _guiController.OnGetScriptEditorControl += _guiController_OnGetScriptEditorControl;
            _agsEditor.PreCompileGame += AGSEditor_PreCompileGame;
            _agsEditor.PreSaveGame += AGSEditor_PreSaveGame;
            _agsEditor.ProcessAllGameTexts += AGSEditor_ProcessAllGameTexts;
			_agsEditor.PreDeleteSprite += AGSEditor_PreDeleteSprite;
            Factory.Events.GamePrepareUpgrade += Events_GamePrepareUpgrade;
            Factory.Events.GamePostLoad += Events_GamePostLoad;
            _modifiedChangedHandler = _loadedRoom_RoomModifiedChanged;
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
            else if (controlID == COMMAND_CHANGE_NUMBER)
            {
                ChangeRoomNumber(_rightClickedRoomNumber);
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
                    TryLoadScriptAndCreateMissing(selectedRoom);
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

        private void TryLoadScriptAndCreateMissing(UnloadedRoom room)
        {
            try
            {
                room.LoadScript();
            }
            catch (FileNotFoundException)
            {
                _guiController.ShowMessage("The script file '" + room.ScriptFileName + "' is missing. An empty script has been created instead.", MessageBoxIcon.Warning);
                room.Script.SaveToDisk(true);
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

        private void ChangeRoomNumber(int roomNumber)
        {
            var room = FindRoomByID(roomNumber);
            int oldNumber = room.Number;
            int newNumber = Factory.GUIController.ShowChangeRoomNumberDialog(oldNumber);
            if (newNumber < 0 || newNumber == oldNumber)
                return;

            RenameRoom(oldNumber, newNumber);
            RoomListTypeConverter.RefreshRoomList();
        }

        protected override void DeleteResourcesUsedByItem(IRoom item)
        {
            DeleteRoom(item);
        }

        private bool DeleteRoom(IRoom roomToDelete)
        {
            UnloadRoom(roomToDelete);

            CloseRoomScriptEditorIfOpen(roomToDelete.Number);

            if (!DeleteRoomFiles(roomToDelete))
                return false;

            RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
            _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
            return true;
        }

        /// <summary>
        /// Deletes any files, both source and compiled ones, related to the given Room.
        /// </summary>
        private bool DeleteRoomFiles(IRoom roomToDelete)
        {
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
                _agsEditor.DeleteFileOnDisk(filesToDelete.ToArray());
            }
            catch (CannotDeleteFileException ex)
            {
                _guiController.ShowMessage("The room file(s) could not be deleted." + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
                return false;
            }

            try
            {
                if (Directory.Exists(roomToDelete.Directory))
                    Directory.Delete(roomToDelete.Directory);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("The room file(s) have been deleted, but could not delete the room directory." + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
            }
            return true;
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

            List<string> newFiles = new List<string>();
            try
            {
                string newFileName = string.Format("room{0}.crm", newRoomNumber);
                string newScriptName = Path.ChangeExtension(newFileName, ".asc");

                if (selectedFileInGameDirectory)
                {
                    if (!File.Exists(newScriptName))
                    {
                        CopyScriptOutOfOldRoomFile(fileName, newScriptName);
                        newFiles.Add(newScriptName);
                    }
					if (newRoomNumber != fileRoomNumber)
					{
						File.Copy(fileName, newFileName, true);
                        newFiles.Add(newFileName);
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

                    newFiles.Add(newFileName);
                    newFiles.Add(newScriptName);
                }

                UnloadedRoom newRoom = new UnloadedRoom(newRoomNumber);
                CompileMessages errors = new CompileMessages();
                Task.WaitAll(ConvertRoomFromCrmToOpenFormat(newRoom, errors, null).ToArray());
                if (errors.HasErrors)
                {
                    _guiController.ShowMessage($"There was an error importing the room. The error was:{Environment.NewLine}{errors.FirstError.AsString}", MessageBoxIcon.Warning);
                    _agsEditor.DeleteFileOnDisk(newFiles.ToArray());
                    return;
                }

                var nodeId = AddSingleItem(newRoom);
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                RePopulateTreeView(nodeId);
                RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
                _guiController.ShowMessage("Room file imported successfully as room " + newRoomNumber + ".", MessageBoxIcon.Information);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("There was a problem importing the room file: " + ex.Message, MessageBoxIcon.Warning);
                _agsEditor.DeleteFileOnDisk(newFiles.ToArray());
            }
        }

        private void CopyScriptOutOfOldRoomFile(string roomFile, string scriptFile)
        {
            string roomScript = null;
            try
            {
                roomScript = _nativeProxy.LoadRoomScript(roomFile);
            }
            catch (Exception e)
            {
                _guiController.ShowMessage($"There was an error loading the script from the old room {roomFile}: {e.Message}", MessageBoxIcon.Warning);
            }

            if (roomScript == null)
            {
                roomScript = "// room script file\r\n";
            }

            try
            {
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

        private bool PromptForAndDeleteAnyExistingRoom(UnloadedRoom room)
        {
            if (Directory.Exists(room.Directory))
            {
                if (_guiController.ShowQuestion($"A \"{room.Directory}\" subdirectory already exists inside this game project. Do you want to overwrite it?", MessageBoxIcon.Warning) == DialogResult.No)
                {
                    return false;
                }

                return DeleteRoom(room);
            }
            return true;
        }

        /// <summary>
        /// Creates empty room definition, not attached to any actual resources yet.
        /// </summary>
        private Room CreateEmptyRoom(int roomNumber, bool genScript, List<Bitmap> backgrounds, List<Bitmap> masks)
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

            if (genScript)
            {
                room.Script = new Script(room.ScriptFileName, "// room script file", false);
            }

            if (backgrounds != null)
            {
                backgrounds.Add(BitmapExtensions.CreateClearBitmap(room.Width, room.Height,
                    Utils.SpriteTools.ColorDepthToPixelFormat(room.ColorDepth), Color.Black));
            }

            if (masks != null)
            {
                Bitmap maskBm = new Bitmap(room.Width, room.Height, PixelFormat.Format8bppIndexed);
                foreach (var m in Enum.GetValues(typeof(RoomAreaMaskType)).Cast<RoomAreaMaskType>())
                {
                    masks.Add(maskBm);
                }
            }

            SyncInteractionScriptModules(room);

            return room;
        }

        private void CreateNewRoom(int roomNumber, RoomTemplate template)
        {
            UnloadedRoom newRoom = new UnloadedRoom(roomNumber);

            if (!PromptForAndDeleteAnyExistingRoom(newRoom))
            {
                return;
            }

            List<string> newFiles = new List<string>(); // these are extra files outside of room dir
            try
            {
                CompileMessages errors = new CompileMessages();
                Directory.CreateDirectory(newRoom.Directory);
				if (template.FileName == null)
				{
                    // Create a default room and save it, generating clear backgrounds and masks
                    List<Bitmap> backgrounds = new List<Bitmap>();
                    List<Bitmap> masks = new List<Bitmap>();
                    Room room = CreateEmptyRoom(roomNumber, true, backgrounds, masks);
                    try
                    {
                        SaveUnloadedRoom(room, backgrounds, masks, errors);
                    }
                    catch (Exception ex)
                    {
                        errors.Add(new CompileError($"Failed to save a new room: {ex.Message}"));
                    }
				}
				else
				{
					_nativeProxy.ExtractRoomTemplateFiles(template.FileName, newRoom.Number, newFiles);
                    Task.WaitAll(ConvertRoomFromCrmToOpenFormat(newRoom, errors, null, template.FileName == null).ToArray());
                }

                if (errors.HasErrors)
                {
                    _guiController.ShowMessage($"There was an error attempting to create the new room. The error was:{Environment.NewLine}{Environment.NewLine}{errors.FirstError.AsString}", MessageBoxIcon.Warning);
                    _agsEditor.DeleteFileOnDisk(newFiles.ToArray());
                    DeleteRoomFiles(newRoom);
                    return;
                }

                string newNodeID = AddSingleItem(newRoom);
                _agsEditor.CurrentGame.FilesAddedOrRemoved = true;
                _guiController.ProjectTree.SelectNode(this, newNodeID);
                RoomListTypeConverter.SetRoomList(_agsEditor.CurrentGame.Rooms);
            }
            catch (Exception ex)
            {
                _guiController.ShowMessage("There was an error attempting to create the new room. The error was: " + ex.Message, MessageBoxIcon.Warning);
                _agsEditor.DeleteFileOnDisk(newFiles.ToArray());
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

                // First test the Room for valid content
                if (!EnsureScriptNamesAreUnique(room, errors))
                    return;

                // Do any automatic fixups that do not raise errors
                // TODO: perhaps a wrong place to do so, find another time and place for this?
                room.Modified |= (PerformPreSaveChecks(room) > 0);

				if (!_agsEditor.AttemptToGetWriteAccess(room.FileName))
				{
					errors.Add(new CompileError("Unable to open file " + room.FileName + " for writing"));
					return;
				}

                if (PreSaveRoom != null)
                {
                    PreSaveRoom(room, errors);
                    if (errors.HasErrors)
                        return;
                }
				
                try
                {
                    BusyDialog.Show(pleaseWaitText, new BusyDialog.ProcessingHandler(SaveRoomOnThread), new CompileRoomParameters(room, errors));
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

        private bool EnsureScriptNamesAreUnique(Room room, CompileMessages errors)
        {
            foreach (RoomHotspot hotspot in room.Hotspots)
            {
                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(hotspot.Name, hotspot) ||
                    room.IsScriptNameAlreadyUsed(hotspot.Name, hotspot))
                {
                    errors.Add(new CompileError("Hotspot '" + hotspot.Name + "' script name conflicts with other game item"));
                }
            }
            foreach (RoomObject obj in room.Objects)
            {
                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(obj.Name, obj) ||
                    room.IsScriptNameAlreadyUsed(obj.Name, obj))
                {
                    errors.Add(new CompileError("Object '" + obj.Name + "' script name conflicts with other game item"));
                }
            }
            return !errors.HasErrors;
        }

        private int PerformPreSaveChecks(Room room)
        {
            int fixups = 0;

            foreach (RoomWalkableArea area in room.WalkableAreas) 
            {
                if (area.UseContinuousScaling)
                {
                    if (area.MaxScalingLevel < area.MinScalingLevel)
                    {
                        int temp = area.MaxScalingLevel;
                        area.MaxScalingLevel = area.MinScalingLevel;
                        area.MinScalingLevel = temp;
                        fixups++;
                    }
                }
            }

            if (room.GameID != _agsEditor.CurrentGame.Settings.UniqueID)
            {
                room.GameID = _agsEditor.CurrentGame.Settings.UniqueID;
                fixups++;
            }

            return fixups;
        }

        private class CompileRoomParameters
        {
            internal Room Room { get; set; }
            internal CompileMessages Errors { get; set; }

            internal CompileRoomParameters(Room room, CompileMessages errors)
            {
                Room = room;
                Errors = errors;
            }
        }

        private object SaveRoomOnThread(IWorkProgress progress, object parameter)
        {
            CompileRoomParameters par = (CompileRoomParameters)parameter;
            Room room = par.Room;
            _agsEditor.RegenerateScriptHeader(room);
            List<Script> headers = (List<Script>)_agsEditor.GetAllScriptHeaders();
            CompileMessages messages = new CompileMessages();
            _agsEditor.CompileScript(room.Script, headers, messages);
            if (messages.HasErrors)
                throw messages.Errors[0];

            ((IRoomController)this).Save();
            
            // Scan after saving, because saving a room here is a more critical task,
            // and scanning is rather a extra aid.
            if (!room.Script.AutoCompleteData.Populated)
                AutoComplete.ConstructCache(room.Script, null);
            ScanAndReportMissingInteractionHandlers(room, par.Errors);

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
                    TryLoadScriptAndCreateMissing(selectedRoom);
                }
                CreateScriptEditor(selectedRoom);
            }

            return _roomScriptEditors[selectedRoom.Number];
        }

        private void CreateScriptEditor(UnloadedRoom selectedRoom)
        {
            ScriptEditor scriptEditor = new ScriptEditor(selectedRoom.Script, _agsEditor, null);
            scriptEditor.RoomNumber = selectedRoom.Number;
            scriptEditor.IsModifiedChanged += ScriptEditor_IsModifiedChanged;
            if (scriptEditor.DockingContainer == null)
            {
                scriptEditor.DockingContainer = new DockingContainer(scriptEditor);
            }
            if ((_loadedRoom != null) && (_loadedRoom.Number == selectedRoom.Number))
            {
                scriptEditor.Room = _loadedRoom;
            }
            _roomScriptEditors[selectedRoom.Number] = new ContentDocument(scriptEditor,
                scriptEditor.GetScriptTabName(), this, SCRIPT_ICON);
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

                treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_NODE + _loadedRoom.Number, _loadedRoom.StateSaving ? ROOM_ICON_SAVE_UNLOADED : ROOM_ICON_UNLOADED);
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
                TryLoadScriptAndCreateMissing(newRoom);
            }
            else if (_roomScriptEditors.ContainsKey(newRoom.Number))
            {
                ((ScriptEditor)_roomScriptEditors[newRoom.Number].Control).UpdateScriptObjectWithLatestTextInWindow();
            }

            _loadedRoom = new Room(LoadData(newRoom)) { Script = newRoom.Script };
            SyncInteractionScriptModules(_loadedRoom); // in case it was broken
            CopyGamePalette();

            UpdateLoadedRoomToTheCurrentVersion(errors);

            // Post-load setup
            LoadImageCache();
            _fileWatchers.Clear();
            _fileWatchers.AddRange(LoadFileWatchers());
            return _loadedRoom;
        }

        private void UpdateLoadedRoomToTheCurrentVersion(CompileMessages errors)
        {
            _loadedRoom.Modified |= ImportExport.CreateInteractionScripts(_loadedRoom, errors);
            _loadedRoom.Modified |= UpgradeFeatures(_loadedRoom, errors);
            if (_loadedRoom.Script.Modified)
            {
                if (_roomScriptEditors.ContainsKey(_loadedRoom.Number))
                {
                    ((ScriptEditor)_roomScriptEditors[_loadedRoom.Number].Control).ScriptModifiedExternally();
                }
            }
        }

        private bool UpgradeFeatures(Room room, CompileMessages errors)
        {
#pragma warning disable 0612
            bool modified = false;
            // Add operations here as necessary
            return modified;
#pragma warning restore 0612
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
					treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_NODE + _loadedRoom.Number, _loadedRoom.StateSaving ? ROOM_ICON_SAVE_LOADED : ROOM_ICON);
					treeController.ChangeNodeIcon(this, TREE_PREFIX_ROOM_SETTINGS + _loadedRoom.Number, ROOM_ICON_LOADED);

					_guiController.ShowOutputPanel(errors);
                    return true;
                }

                return false;
            }
        }

        private string GetRoomSettingsTabName()
        {
            if(!string.IsNullOrEmpty(_loadedRoom.Description))
            {
                return "Room " + _loadedRoom.Number.ToString() + (_loadedRoom.Modified ? " *" : "") + ": " + _loadedRoom.Description;
            }

            return "Room " + _loadedRoom.Number + (_loadedRoom.Modified ? " *" : "");
        }

        private void CreateRoomSettings(DockData previousDockData)
        {
            string paneTitle = GetRoomSettingsTabName();

            RoomSettingsEditor editor = new RoomSettingsEditor(_loadedRoom, this);
            _roomSettings = new ContentDocument(editor,
                paneTitle, this, ROOM_ICON_LOADED, ConstructPropertyObjectList(_loadedRoom));
            if (previousDockData != null && previousDockData.DockState != DockingState.Document)
            {
                _roomSettings.PreferredDockData = previousDockData;
            }
            _roomSettings.SelectedPropertyGridObject = _loadedRoom;
            editor.SaveRoom += RoomEditor_SaveRoom;
            editor.AbandonChanges += RoomsComponent_AbandonChanges;
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
            _roomSettings.Name = GetRoomSettingsTabName();
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
            if (_guiController.ActivePane.SelectedPropertyGridObject is Room)
            {
                RoomPropertyChanged(propertyName, oldValue);
            }
            else if (_guiController.ActivePane.SelectedPropertyGridObject is Character)
            {
                // TODO: wish we could forward event to the CharacterComponent.OnPropertyChanged,
                // but its implementation relies on it being active Pane!
                CharacterPropertyChanged(propertyName, oldValue);
            }
        }

        private void RoomPropertyChanged(string propertyName, object oldValue)
        {
            if ((propertyName == UnloadedRoom.PROPERTY_NAME_DESCRIPTION) && (_loadedRoom != null))
            {
                UnloadedRoom room = FindRoomByID(_loadedRoom.Number);
                room.Description = _loadedRoom.Description;
                RePopulateTreeView(GetItemNodeID(room));
                RoomListTypeConverter.RefreshRoomList();
                ContentDocument doc;
                if (_roomScriptEditors.TryGetValue(_loadedRoom.Number, out doc) && doc != null)
                {
                    ScriptEditor scriptEditor = ((ScriptEditor)doc.Control);
                    UpdateScriptWindowTitle(scriptEditor);
                }
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
        }

        private void CharacterPropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == Character.PROPERTY_NAME_SCRIPTNAME)
            {
                // TODO: re-investigate if we can allow this
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
			else
			{
                bool wasThisRoomLoaded = (_loadedRoom != null) && (_loadedRoom.Number == currentNumber);
                if (wasThisRoomLoaded)
                {
                    if (!SaveRoomAlwaysAndShowErrors(_loadedRoom, _roomSettings?.Control as RoomSettingsEditor))
                    {
                        return;
                    }

                    CloseRoomScriptEditorIfOpen(currentNumber);
                    UnloadCurrentRoom();
                }

                UnloadedRoom oldRoom = FindRoomByID(currentNumber);
                UnloadedRoom tempNewRoom = new UnloadedRoom(numberRequested);
                string oldRoomDirectory = oldRoom.Directory;
                string newRoomDirectory = tempNewRoom.Directory;

                // Rename compiled room file (it's in the project root)
                _agsEditor.RenameFileOnDisk(oldRoom.FileName, tempNewRoom.FileName);

                // Create new room directory, and move standard room files there,
                // renaming according to the new room number
                _agsEditor.CreateDirectoryInProject(tempNewRoom.Directory);
                _agsEditor.RenameFileOnDisk(oldRoom.UserFileName, tempNewRoom.UserFileName);
                _agsEditor.RenameFileOnDisk(oldRoom.ScriptFileName, tempNewRoom.ScriptFileName);
                _agsEditor.RenameFileOnDisk(oldRoom.DataFileName, tempNewRoom.DataFileName);

                for (int i = 0; i < Room.MAX_BACKGROUNDS; i++)
                {
                    string oldBackgroundFileName = oldRoom.GetBackgroundFileName(i);
                    if (File.Exists(oldBackgroundFileName))
                        _agsEditor.RenameFileOnDisk(oldBackgroundFileName, tempNewRoom.GetBackgroundFileName(i));
                }

                foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)).Cast<RoomAreaMaskType>().Where(t => t != RoomAreaMaskType.None))
                {
                    string oldMaskFileName = oldRoom.GetMaskFileName(mask);
                    if (File.Exists(oldMaskFileName))
                        _agsEditor.RenameFileOnDisk(oldMaskFileName, tempNewRoom.GetMaskFileName(mask));
                }

                // Load room data, change the number (and anything else if it's required),
                // and save updated data back
                oldRoom.Number = numberRequested;
                XDocument newRoomXml = XDocument.Load(tempNewRoom.DataFileName);
                newRoomXml.Element("Room").SetElementValue("Number", numberRequested);
                using (var writer = new XmlTextWriter(tempNewRoom.DataFileName, Types.Utilities.UTF8))
                {
                    writer.Formatting = Formatting.Indented;
                    newRoomXml.Save(writer);
                }

                // Move any other custom files found in the old room dir, and delete the old dir
                Utilities.SafeMoveDirectoryFiles(Path.Combine(_agsEditor.GameDirectory, oldRoomDirectory),
                    Path.Combine(_agsEditor.GameDirectory, newRoomDirectory));

                if (wasThisRoomLoaded)
                {
                    LoadDifferentRoom(oldRoom);
                    _roomSettings.TreeNodeID = TREE_PREFIX_ROOM_SETTINGS + numberRequested;
                    _guiController.AddOrShowPane(_roomSettings);
                }
				_agsEditor.CurrentGame.FilesAddedOrRemoved = true;
				RePopulateTreeView(GetItemNodeID(oldRoom));
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
                menu.Add(new MenuCommand(COMMAND_CHANGE_NUMBER, "Change room's number", null));
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
            int roomNumberToEdit = -1;
            if ((fileName == Script.CURRENT_ROOM_SCRIPT_FILE_NAME) &&
               (_loadedRoom != null))
            {
                roomNumberToEdit = _loadedRoom.Number;
            }
            else
            {
                // FIXME: backslashes will work only on MS Windows!
                Match match = Regex.Match(fileName, @"^Rooms\\+(\d+)\\+room\1\.asc$", RegexOptions.IgnoreCase);
                if ((match.Success) && (match.Groups.Count == 2))
                {
                    roomNumberToEdit = Convert.ToInt32(match.Groups[1].Captures[0].Value);
                }
            }

            // CHECKME: find out why is was necessary, and comment
            foreach (ContentDocument doc in _roomScriptEditors.Values)
            {
                if (isDebugExecutionPoint)
                {
                    ((ScriptEditor)doc.Control).RemoveExecutionPointMarker();
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
            if (evArgs.Handled)
            {
                return; // operation has been completed by another handler
            }

            int roomNumberToEdit = GetRoomNumberForFileName(evArgs.FileName, evArgs.IsDebugExecutionPoint);
            
            if (roomNumberToEdit >= 0)
            {
                UnloadedRoom roomToGetScriptFor = GetUnloadedRoom(roomNumberToEdit);                
                ScriptEditor editor = (ScriptEditor)CreateOrShowScript(roomToGetScriptFor).Control;
                if (editor != null)
                {
                    ZoomToCorrectPositionInScript(editor, evArgs);
                    return;
                }
            }

            evArgs.Result = ZoomToFileResult.ScriptNotFound;
        }

        private void GUIController_OnGetScript(string fileName, ref Script script)
        {
            if (_loadedRoom == null)
                return;

            if ((fileName == Script.CURRENT_ROOM_SCRIPT_FILE_NAME) ||
                (fileName == _loadedRoom.ScriptFileName))
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
                if (!File.Exists(script.Header.FileName))
                {
                    continue; // project file is missing, undefined behavior
                }

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

            // First of all test all rooms for validity of data on disk
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
                    errors.Add(new CompileError("File not found: " + unloadedRoom.ScriptFileName + "; If you deleted this file, delete the room in the Project Explorer using a context menu."));
                    success = false;
                }
                else if (!File.Exists(unloadedRoom.DataFileName))
                {
                    errors.Add(new CompileError("File not found: " + unloadedRoom.DataFileName + "; If you deleted this file, delete the room from the Project Explorer using a context menu."));
                    success = false;
                }
                else if (!File.Exists(unloadedRoom.GetBackgroundFileName(0)))
                {
                    errors.Add(new CompileError("File not found: " + unloadedRoom.GetBackgroundFileName(0) + "; If you deleted this file, delete the room from the Project Explorer using a context menu."));
                    success = false;
                }
                else if (missingMasks.Any())
                {
                    errors.AddRange(missingMasks.Select(f => new CompileError("File not found: " + f + "; If you deleted this file, delete the room from the Project Explorer using a context menu.")));
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

            // Try to ensure write access to all gathered output files
			if (!_agsEditor.AttemptToGetWriteAccess(roomFileNamesToRebuild))
			{
				errors.Add(new CompileError("Failed to open files for writing"));
				return false;
			}

            string rebuildReason = rebuildAll ? "because the full rebuild was ordered" : "because a script has changed";

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

				CompileMessages roomErrors = new CompileMessages();
				SaveRoomButDoNotShowAnyErrors(room, roomErrors, $"Rebuilding room {room.Number} {rebuildReason}...");

				if (roomErrors.HasErrors)
				{
					errors.Add(new CompileError($"Failed to save room {room.FileName}; details below"));
					errors.AddRange(roomErrors);
					success = false;
					break;
				}
                else if (roomErrors.Count > 0)
                {
                    errors.Add(new CompileWarning($"Room {room.FileName} was saved, but there were warnings; details below"));
                    errors.AddRange(roomErrors);
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

        private void Events_GamePrepareUpgrade(UpgradeGameEventArgs args)
        {
            args.Tasks.Add(new UpgradeGameRoomsOpenFormatTask(ConvertAllRoomsFromCrmToOpenFormat));
        }

        private void Events_GamePostLoad(Game game)
        {
            // For the same reason we do not upgrade the room data right away,
            // but check if they need to be upgraded (e.g. because of a new project version),
            // and mark the project as requiring a full rebuild. This is a workaround, which
            // will trigger all rooms recompilation, forcing them to load and upgrade
            // whenever user compiles the game.
            if (IsRoomUpgradeNecessary(game))
            {
                game.WorkspaceState.RequiresRebuild = true;
            }
            //UpgradeAllRoomsIfNecessary(game);
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
            string nodeIconName = room.StateSaving ? ROOM_ICON_SAVE_UNLOADED : ROOM_ICON_UNLOADED;
            string iconName = ROOM_ICON_UNLOADED;

            if ((_loadedRoom != null) && (room.Number == _loadedRoom.Number))
			{
                nodeIconName = room.StateSaving ? ROOM_ICON_SAVE_LOADED : ROOM_ICON_LOADED;
                iconName = ROOM_ICON_LOADED;
			}

			ProjectTree treeController = _guiController.ProjectTree;
			ProjectTreeItem treeItem = treeController.AddTreeBranch(this, GetItemNodeID(room), GetItemNodeLabel(room), nodeIconName);
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
                if (_loadedRoom.Modified)
                {
                    SaveImages(false);
                    using (var writer = new XmlTextWriter(_loadedRoom.DataFileName, Types.Utilities.UTF8))
                    {
                        writer.Formatting = Formatting.Indented;
                        _loadedRoom.ToXmlDocument().Save(writer);
                    }
                    _loadedRoom.Modified = false;
                }
                IsBeingSaved = false;
                LastSavedAt = DateTime.Now;
            });

            SaveCrm();
        }

        Bitmap IRoomController.GetBackground(int background)
        {
            if (_loadedRoom == null)
            {
                throw new InvalidOperationException("No room is currently loaded");
            }

            return _backgroundCache[background].Image.Clone() as Bitmap;
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

            // Make certain that the bitmap's color depth is compatible with the AGS
            Bitmap newBmp = bmp.CloneAsAGSCompatible();
            bool hasResolutionChanged = _loadedRoom.Width != newBmp.Width || _loadedRoom.Height != newBmp.Height;
            _loadedRoom.Width = newBmp.Width;
            _loadedRoom.Height = newBmp.Height;
            _loadedRoom.ColorDepth = newBmp.GetColorDepth();

            if (_loadedRoom.ColorDepth == 8)
            {
                int colorsImage, colorsLimit;
                CopyGamePalette(); // in case they had changes to game colors in the meantime
                PaletteUtilities.RemapBackground(newBmp, !Factory.AGSEditor.Settings.RemapPalettizedBackgrounds, _roomPalette, out colorsImage, out colorsLimit);
                if (background == 0)
                {
                    newBmp.CopyToAGSBackgroundPalette(_roomPalette); // update current room palette
                    Factory.NativeProxy.ApplyPalette(_roomPalette); // sync native palette
                }

                // TODO: not sure if it's good to report error here, in the interface impl,
                // but the existing method does not provide a "CompileMessages" arg
                if (Factory.AGSEditor.Settings.RemapPalettizedBackgrounds &&
                    (colorsImage > colorsLimit))
                {
                    _guiController.ShowMessage($"The image uses more colors ({colorsImage}) than palette slots allocated to backgrounds ({colorsLimit}). Some colours will be lost.",
                        MessageBoxIconType.Information);
                }
            }

            if (background >= _backgroundCache.Count)
            {
                _backgroundCache.Add(new RoomImage(newBmp, true));
                _fileWatchers[_loadedRoom.GetBackgroundFileName(_loadedRoom.BackgroundCount++)].Enabled = true;
            }
            else
            {
                _backgroundCache[background]?.Dispose();
                _backgroundCache[background] = new RoomImage(newBmp, true);
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

            return _maskCache[mask].Image.Clone() as Bitmap;
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

            SetMaskDirect(mask, bmp.Clone() as Bitmap, true);
        }

        void SetMaskDirect(RoomAreaMaskType mask, Bitmap bmp, bool markModified)
        {
            if (ValidateMask(mask, bmp))
            {
                RoomImage toDispose;
                _maskCache.TryGetValue(mask, out toDispose);
                toDispose?.Dispose();
                _maskCache[mask] = new RoomImage(bmp, markModified);
                _loadedRoom.Modified |= markModified;
            }
            else // invalid source, try to recover
            {
                if (_maskCache.ContainsKey(mask) && _maskCache[mask] != null)
                    return; // there's already a previous version in cache, no need to do anything
                // create an empty mask
                _maskCache[mask] = new RoomImage(CreateMaskBitmap(mask, _loadedRoom.Width, _loadedRoom.Height), true);
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

            Bitmap mask = _maskCache[maskType].Image;

            if (x < 0 || y < 0 || x > mask.Width || y > mask.Height) return 0;

            return mask.GetRawData()[(y * mask.Width) + x];
        }

        void IRoomController.ResetMasks()
        {
            foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
            {
                if (mask == RoomAreaMaskType.None)
                    continue;

                if (_maskCache.ContainsKey(mask))
                {
                    _maskCache[mask]?.Dispose();
                    _maskCache[mask] = new RoomImage(CreateMaskBitmap(mask, _loadedRoom.Width, _loadedRoom.Height), true);
                }
            }
        }

        void IRoomController.ResizeMasks(bool doScale, int newWidth, int newHeight, int xOffset, int yOffset)
        {   
            if (newWidth <= 0 || newHeight <= 0)
            {
                throw new ArgumentException("Negative width or height set for mask resize.");
            }

            foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
            {
                if (mask == RoomAreaMaskType.None)
                    continue;

                if (_maskCache.ContainsKey(mask))
                {
                    _maskCache[mask].Image = ResizeMaskBitmap(_maskCache[mask].Image, mask, doScale, newWidth, newHeight, xOffset, yOffset);
                    _maskCache[mask].Modified = true;
                }
            }
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

            Bitmap background = _backgroundCache[backgroundNumber].Image;

            // x and y is already scaled from outside the method call
            Rectangle drawingArea = new Rectangle(
                point.X, point.Y, (int)(background.Width * scaleFactor), (int)(background.Height * scaleFactor));

            g.DrawImage(background, drawingArea);

            if (maskType != RoomAreaMaskType.None)
            {
                Bitmap mask8bpp = _maskCache[maskType].Image;
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
                    // NOTE: we're using default game palette for displaying masks
                    if (selectedArea > 0)
                    {
                        // if a bright colour, use it, else, draw in red
                        paletteDrawing.Entries[selectedArea] = selectedArea < 15 && selectedArea != 7 && selectedArea != 8
                            ? _agsEditor.CurrentGame.Palette[selectedArea].Colour
                            : Color.FromArgb(255, 0, 0);
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
            Bitmap bg = _backgroundCache[0].Image;
            int baseWidth = bg.Width;
            int baseHeight = bg.Height;

            foreach (RoomAreaMaskType maskType in Enum.GetValues(typeof(RoomAreaMaskType)))
            {
                if (maskType == RoomAreaMaskType.None) continue;

                using (RoomImage mask = _maskCache[maskType])
                {
                    double scale = _loadedRoom.GetMaskScale(maskType);
                    _maskCache[maskType] = new RoomImage(mask.Image.ScaleIndexed((int)(baseWidth * scale), (int)(baseHeight * scale)), true);
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

        private string GetItemNodeID(IRoom room)
        {
            return TREE_PREFIX_ROOM_NODE + room.Number;
        }

        private string GetItemNodeLabel(IRoom room)
        {
            return room.Number.ToString() + ": " + room.Description;
        }

        /// <summary>
        /// Helper class for use when scanning for event handlers
        /// </summary>
        private class RoomObjectWithEvents
        {
            public Room Room;
            // TODO: add parent class to room objects (and maybe game objects in general)
            // with overridden properties like "ScriptName", "ID" and "DisplayName",
            // this will allow to get these basic values without casting to explicit types.
            public RoomObject Object;
            public RoomHotspot Hotspot;
            public RoomRegion Region;

            public RoomObjectWithEvents(Room room) { Room = room; }
            public RoomObjectWithEvents(RoomObject roomObject) { Object = roomObject; }
            public RoomObjectWithEvents(RoomHotspot roomObject) { Hotspot = roomObject; }
            public RoomObjectWithEvents(RoomRegion roomObject) { Region = roomObject; }
        }

        private class RoomEventReference
        {
            public RoomObjectWithEvents RoomObject;
            public string TypeName;
            public int ID;
            public string ObjName;
            public string EventName;
            public string FunctionName;

            public RoomEventReference(RoomObjectWithEvents obj, string evtName, string fnName)
            {
                RoomObject = obj;
                EventName = evtName;
                FunctionName = fnName;

                if (RoomObject.Room != null)
                {
                    ObjName = "Room";
                }
                else if (RoomObject.Object != null)
                {
                    TypeName = "Object";
                    ID = RoomObject.Object.ID;
                    ObjName = RoomObject.Object.Name;
                }
                else if (RoomObject.Hotspot != null)
                {
                    TypeName = "Hotspot";
                    ID = RoomObject.Hotspot.ID;
                    ObjName = RoomObject.Hotspot.Name;
                }
                else if (RoomObject.Region != null)
                {
                    TypeName = "Region";
                    ID = RoomObject.Region.ID;
                    ObjName = $"Region{RoomObject.Region.ID}";
                }
            }
        }

        private void ScanAndReportMissingInteractionHandlers(Room room, CompileMessages errors)
        {
            // Gather function names from the Room and all of their contents,
            // in order to check missing functions in a single batch.
            List<RoomEventReference> objectEvents = new List<RoomEventReference>();
            objectEvents.AddRange(
                room.Interactions.ScriptFunctionNames.Select((fn, i) =>
                   new RoomEventReference(new RoomObjectWithEvents(room), room.Interactions.FunctionSuffixes[i], room.Interactions.ScriptFunctionNames[i])));

            foreach (var obj in room.Objects)
            {
                var objWithEvents = new RoomObjectWithEvents(obj);
                objectEvents.AddRange(
                    obj.Interactions.ScriptFunctionNames.Select((fn, i) =>
                        new RoomEventReference(objWithEvents, obj.Interactions.FunctionSuffixes[i], obj.Interactions.ScriptFunctionNames[i])));
            }
            foreach (var hot in room.Hotspots)
            {
                var objWithEvents = new RoomObjectWithEvents(hot);
                objectEvents.AddRange(
                    hot.Interactions.ScriptFunctionNames.Select((fn, i) =>
                        new RoomEventReference(objWithEvents, hot.Interactions.FunctionSuffixes[i], hot.Interactions.ScriptFunctionNames[i])));
            }
            foreach (var reg in room.Regions)
            {
                var objWithEvents = new RoomObjectWithEvents(reg);
                objectEvents.AddRange(
                    reg.Interactions.ScriptFunctionNames.Select((fn, i) =>
                        new RoomEventReference(objWithEvents, reg.Interactions.FunctionSuffixes[i], reg.Interactions.ScriptFunctionNames[i])));
            }

            var functionNames = objectEvents.Select(evt => !string.IsNullOrEmpty(evt.FunctionName) ? evt.FunctionName : $"{evt.ObjName}_{evt.EventName}");
            var funcs = _agsEditor.Tasks.FindEventHandlers(room.Script.FileName, room.Script.AutoCompleteData, functionNames.ToArray());
            if (funcs == null || funcs.Length == 0)
                return;

            for (int i = 0; i < funcs.Length; ++i)
            {
                RoomEventReference evtRef = objectEvents[i];
                RoomObjectWithEvents roomObject = evtRef.RoomObject;
                bool has_interaction = !string.IsNullOrEmpty(evtRef.FunctionName);
                bool has_function = funcs[i].HasValue;
                // If we have an assigned interaction function, but the function is not found - report a missing warning
                if (has_interaction && !has_function)
                {
                    if (roomObject.Room != null)
                    {
                        errors.Add(new CompileWarning($"Room {room.Number}'s event {evtRef.EventName} function \"{evtRef.FunctionName}\" not found in script {room.ScriptFileName}."));
                    }
                    else
                    {
                        errors.Add(new CompileWarning($"Room {room.Number}: {evtRef.TypeName} ({evtRef.ID}) {evtRef.ObjName}'s event {evtRef.EventName} function \"{evtRef.FunctionName}\" not found in script {room.ScriptFileName}."));
                    }
                }
                // If we don't have an assignment, but has a similar function - report a possible unlinked function
                else if (!has_interaction && has_function)
                {
                    if (roomObject.Room != null)
                    {
                        errors.Add(new CompileWarningWithFunction($"Function \"{funcs[i].Value.Name}\" looks like an event handler, but is not linked on Room {room.Number}'s Event pane",
                            funcs[i].Value.ScriptName, funcs[i].Value.Name, funcs[i].Value.LineNumber));
                    }
                    else
                    {
                        errors.Add(new CompileWarningWithFunction($"Function \"{funcs[i].Value.Name}\" looks like an event handler, but is not linked on {evtRef.TypeName} ({evtRef.ID}) {evtRef.ObjName}'s Event pane",
                            funcs[i].Value.ScriptName, funcs[i].Value.Name, funcs[i].Value.LineNumber));
                    }
                }
            }
        }

        private void CopyGamePalette()
        {
            _roomPalette = new PaletteEntry[Factory.AGSEditor.CurrentGame.Palette.Length];
            Array.Copy(Factory.AGSEditor.CurrentGame.Palette, _roomPalette, Factory.AGSEditor.CurrentGame.Palette.Length);
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
                    _backgroundCache.Add(new RoomImage(LoadBackground(i), false));
                }
            }

            bool imageNotFound = false;

            if (!_backgroundCache.Any())
            {
                _backgroundCache.Add(new RoomImage(new Bitmap(_loadedRoom.Width, _loadedRoom.Height), true));
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
            
            _loadedRoom.ColorDepth = _backgroundCache[0].Image.GetColorDepth();

            foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
            {
                if (mask == RoomAreaMaskType.None)
                {
                    continue;
                }

                if (File.Exists(_loadedRoom.GetMaskFileName(mask)))
                {
                    SetMaskDirect(mask, LoadMask(mask), false);
                }
                else
                {
                    _maskCache[mask] = new RoomImage(CreateMaskBitmap(mask, _loadedRoom.Width, _loadedRoom.Height), true);
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

        private Bitmap LoadBackground(int i)
        {
            Bitmap newBmp = BitmapExtensions.LoadNonLockedBitmap(_loadedRoom.GetBackgroundFileName(i));
            // For 8-bit rooms - remap loaded background images
            if (_loadedRoom.ColorDepth == 8)
            {
                int colorsImage, colorsLimit;
                CopyGamePalette(); // in case they had changes to game colors in the meantime
                // Note we always do exact palette here, assuming that the background
                // stored in the project files is already in a wanted palette
                PaletteUtilities.RemapBackground(newBmp, true /* exact palette */, _roomPalette, out colorsImage, out colorsLimit);
                if (i == 0)
                {
                    newBmp.CopyToAGSBackgroundPalette(_roomPalette); // update current room palette
                    Factory.NativeProxy.ApplyPalette(_roomPalette); // sync native palette
                }
            }
            return newBmp;
        }

        private void RefreshBackground(int i)
        {
            _backgroundCache[i]?.Dispose();
            _backgroundCache[i] = new RoomImage(LoadBackground(i), false);
            ((RoomSettingsEditor)_roomSettings.Control).InvalidateDrawingBuffer();
            _loadedRoom.Modified = true;
        }

        /// <summary>
        /// Creates mask bitmap for the given mask type of the requested size.
        /// </summary>
        private Bitmap CreateMaskBitmap(RoomAreaMaskType mask, int width, int height)
        {
            double scale = _loadedRoom.GetMaskScale(mask);
            var bitmap = new Bitmap((int)(width * scale), (int)(height * scale), PixelFormat.Format8bppIndexed);
            bitmap.SetFromAGSPalette(Factory.AGSEditor.CurrentGame.Palette); // enforce default mask palette
            return bitmap;
        }

        /// <summary>
        /// Resizes a mask bitmap for the given mask type of the requested size.
        /// </summary>
        private Bitmap ResizeMaskBitmap(Bitmap originalBitmap, RoomAreaMaskType mask, bool doScale, int newWidth, int newHeight, int xOffset, int yOffset)
        {
            // Adjust by the mask scale
            double scale = _loadedRoom.GetMaskScale(mask);

            int drawWidth = doScale ? newWidth : originalBitmap.Width;
            int drawHeight = doScale ? newHeight : originalBitmap.Height;

            return BitmapExtensions.ResizeScaleAndOffset(originalBitmap, drawWidth, drawHeight, (int)(newWidth * scale), (int)(newHeight * scale), xOffset, yOffset);
        }

        private Bitmap LoadMask(RoomAreaMaskType mask) => BitmapExtensions.LoadNonLockedBitmap(_loadedRoom.GetMaskFileName(mask));

        private void RefreshMask(RoomAreaMaskType mask)
        {
            SetMaskDirect(mask, LoadMask(mask), false);
            ((RoomSettingsEditor)_roomSettings.Control).InvalidateDrawingBuffer();
            _loadedRoom.Modified = true;
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

            // normalize palette because a png can have partial palettes
            newMask.Palette = BitmapExtensions.CreateColorPalette();
            newMask.SetFromAGSPalette(Factory.AGSEditor.CurrentGame.Palette); // enforce default mask palette

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

        private void SaveImages(bool forced)
        {
            lock (_loadedRoom)
            {
                for (int i = 0; i < Room.MAX_BACKGROUNDS; i++)
                {
                    string fileName = _loadedRoom.GetBackgroundFileName(i);
                    // Delete remnants of unused backgrounds
                    if (i >= _backgroundCache.Count)
                    {
                        Utilities.TryDeleteFile(fileName);
                        continue;
                    }

                    if (forced || _backgroundCache[i].Modified)
                    {
                        _backgroundCache[i].Image.Save(fileName, ImageFormat.Png);
                        _backgroundCache[i].Modified = false;
                    }
                }

                foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
                {
                    if (mask == RoomAreaMaskType.None)
                        continue;

                    var maskObj = _maskCache[mask];
                    if (!forced && !maskObj.Modified)
                        continue;

                    string fileName = _loadedRoom.GetMaskFileName(mask);
                    maskObj.Image.Save(fileName, ImageFormat.Png);
                    maskObj.Modified = false;
                }
            }
        }

        /// <summary>
        /// Saves room files without loading a room into the editor.
        /// Does no validity checks, and does not compile any data.
        /// NOTE: this is necessary to have at the moment, because RoomComponent
        /// works with only 1 loaded room at a time. Should be revisited in case
        /// we support multiple simultaneous edited rooms.
        /// </summary>
        private void SaveUnloadedRoom(Room room, List<Bitmap> backgrounds, List<Bitmap> masks,
            CompileMessages errors)
        {
            // Save room's data.xml
            using (var writer = new XmlTextWriter(room.DataFileName, Types.Utilities.UTF8))
            {
                writer.Formatting = Formatting.Indented;
                room.ToXmlDocument().Save(writer);
            }

            // Save room's script
            if (room.Script != null)
            {
                room.Script.SaveToDisk(true);
            }

            // Save room's backgrounds and mask images
            if (backgrounds != null)
            {
                foreach (var bg in backgrounds.Select((bm, i) => new { i, bm }))
                {
                    bg.bm.Save(room.GetBackgroundFileName(bg.i));
                }
            }

            if (masks != null)
            {
                foreach (var m in 
                    Enum.GetValues(typeof(RoomAreaMaskType))
                        .Cast<RoomAreaMaskType>()
                        .Where(m => m != RoomAreaMaskType.None && (int)m < masks.Count))
                {
                    masks[(int)m].Save(room.GetMaskFileName(m));
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

        #region Upgrade Rooms to a new version

        /// <summary>
        /// Checks the loaded game's version and tells whether all rooms has to be upgraded.
        /// </summary>
        private bool IsRoomUpgradeNecessary(Game game)
        {
            // Test the game version here and decide if upgrade is needed
            return (game.SavedRoomXmlVersion == null)
                || (game.SavedRoomXmlVersion < new System.Version(Room.LATEST_XML_VERSION));
        }

        /// <summary>
        /// Checks the loaded game's version and does the room upgrade process if it's necessary.
        /// </summary>
        private async void UpgradeAllRoomsIfNecessary(Game game)
        {
            if (!IsRoomUpgradeNecessary(game))
                return;

            // Do the upgrade process
            IList<IRoom> rooms = _agsEditor.CurrentGame.Rooms;
            object progressLock = new object();
            string progressText = "Upgrading rooms.";
            CompileMessages errors = new CompileMessages();
            using (Progress progressForm = new Progress(rooms.Count, progressText))
            {
                progressForm.Show();
                int progress = 0;
                Action progressReporter = () =>
                {
                    lock (progressLock) { progress++; }
                    progressForm.SetProgress(progress, $"{progressText} {progress} of {rooms.Count} rooms upgraded.");
                };

                var roomsUpgradingTasks = rooms
                    .Cast<UnloadedRoom>()
                    .SelectMany(r => UpgradeRoomToNewVersion(r, errors, progressReporter))
                    .ToArray();
                await Task.WhenAll(roomsUpgradingTasks);
            }
        }

        /// <summary>
        /// Converts a single room from .crm to open format.
        /// </summary>
        /// <param name="room">The room to convert to open format</param>
        /// <returns>A collection of tasks that converts the room async.</returns>
        private IEnumerable<Task> UpgradeRoomToNewVersion(UnloadedRoom unloadedRoom, CompileMessages errors,
            Action report = null)
        {
            // Load room data into memory
            Room room = new Room(LoadData(unloadedRoom));

            // Do upgrade
            SyncInteractionScriptModules(room); // in case it was broken
            UpgradeFeatures(room, errors);

            // Save the room data back
            yield return SaveXmlAsync(room.ToXmlDocument(), room.DataFileName);

            report?.Invoke();
        }

        #endregion // Upgrade Rooms to a new version

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
        private async void ConvertAllRoomsFromCrmToOpenFormat(Game game, IWorkProgress progress, CompileMessages errors)
        {
            if ((_agsEditor.CurrentGame.SavedXmlVersion >= new System.Version(AGSEditor.FIRST_XML_VERSION_WITHOUT_INDEX)) ||
                _agsEditor.CurrentGame.SavedXmlVersionIndex >= AGSEditor.AGS_4_0_0_XML_VERSION_INDEX_OPEN_ROOMS)
                return; // Upgrade already completed

            IList<IRoom> rooms = _agsEditor.CurrentGame.Rooms;

            // If the room directory we want to write to already exists then backup
            if (Directory.Exists(UnloadedRoom.ROOM_DIRECTORY))
            {
                string backupRootDir = Utilities.MakeUniqueDirectory(_agsEditor.CurrentGame.DirectoryPath, UnloadedRoom.ROOM_DIRECTORY, "Backup-");
                Utilities.SafeMoveDirectoryFiles(UnloadedRoom.ROOM_DIRECTORY, backupRootDir);
            }

            // Now upgrade
            object progressLock = new object();
            string progressText = "Converting rooms from .crm to open format.";
            {
                // FIXME: we need sub-task support in the IWorkProgress interface, and respective implementation!
                progress.Total = rooms.Count;
                int progressCounter = 0;
                Action progressReporter = () =>
                {
                    lock (progressLock)
                    {
                        progressCounter++;
                        progress.Current = progressCounter;
                        progress.Message = $"{progressText} {progressCounter} of {rooms.Count} rooms converted.";
                    }
                };

                var roomsConvertingTasks = rooms
                    .Cast<UnloadedRoom>()
                    .SelectMany(r => ConvertRoomFromCrmToOpenFormat(r, errors, progressReporter))
                    .ToArray();
                await Task.WhenAll(roomsConvertingTasks);
            }
        }

        /// <summary>
        /// Converts a single room from .crm to open format.
        /// </summary>
        /// <param name="room">The room to convert to open format</param>
        /// <returns>A collection of tasks that converts the room async.</returns>
        private IEnumerable<Task> ConvertRoomFromCrmToOpenFormat(UnloadedRoom unloadedRoom, CompileMessages errors,
            Action report = null, bool isNewRoom=false)
        {
            Native.NativeRoom nativeRoom = null;
            try
            {
                nativeRoom = new Native.NativeRoom(unloadedRoom.FileName, null);
            }
            catch (Exception e)
            {
                errors.Add(new CompileError($"Failed to load a room from {unloadedRoom.FileName}", e));
                report?.Invoke();
                yield break;
            }

            Room room = nativeRoom.ConvertToManagedRoom(unloadedRoom.Number, null);
            room.Description = unloadedRoom.Description;
            room.Script = unloadedRoom.Script;

            // Adjust all Interactions to have a new script module name
            SyncInteractionScriptModules(room);

            // Create a directory for this room
            Directory.CreateDirectory(room.Directory);

            for (int i = 0; i < room.BackgroundCount; i++)
                yield return SaveAndDisposeBitmapAsync(nativeRoom.GetBackground(i), room.GetBackgroundFileName(i));

            yield return SaveXmlAsync(room.ToXmlDocument(), room.DataFileName);

            foreach (RoomAreaMaskType type in Enum.GetValues(typeof(RoomAreaMaskType)).Cast<RoomAreaMaskType>().Where(m => m != RoomAreaMaskType.None))
                yield return SaveAndDisposeBitmapAsync(nativeRoom.GetAreaMask(type), room.GetMaskFileName(type));

            if (!isNewRoom)
            {
                string oldScriptFileName = $"room{room.Number}.asc";
                if (File.Exists(oldScriptFileName))
                    File.Move(oldScriptFileName, room.ScriptFileName);

                string oldUserFileName = $"room{room.Number}.crm.user";
                if (File.Exists(oldUserFileName))
                    File.Move(oldUserFileName, room.UserFileName);
            }

            nativeRoom.Dispose();

            report?.Invoke();
        }

        /// <summary>
        /// Synchronizes ScriptModule property in all the Interaction elements
        /// within a Room and room objects.
        /// </summary>
        private void SyncInteractionScriptModules(Room room)
        {
            room.Interactions.ScriptModule = room.ScriptFileName;
            foreach (var obj in room.Objects)
                obj.Interactions.ScriptModule = room.ScriptFileName;
            foreach (var hot in room.Hotspots)
                hot.Interactions.ScriptModule = room.ScriptFileName;
            foreach (var reg in room.Regions)
                reg.Interactions.ScriptModule = room.ScriptFileName;
        }

        private Task SaveXmlAsync(XmlDocument document, string filename) => Task.Run(() =>
        {
            using (var writer = new XmlTextWriter(filename, Types.Utilities.UTF8))
            {
                writer.Formatting = Formatting.Indented;
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

            // Sync native palette before writing
            if ((_loadedRoom.ColorDepth == 8) && (_loadedRoom.BackgroundCount > 0))
            {
                CopyGamePalette(); // in case they had changes to game colors in the meantime
                _backgroundCache[0].Image.CopyToAGSBackgroundPalette(_roomPalette); // update current room palette
                Factory.NativeProxy.ApplyPalette(_roomPalette); // sync native palette
            }

            using (var nativeRoom = new Native.NativeRoom(_loadedRoom))
            {
                for (int i = 0; i < _loadedRoom.BackgroundCount; i++)
                {
                    nativeRoom.SetBackground(i, _backgroundCache[i].Image);
                }

                foreach (RoomAreaMaskType mask in Enum.GetValues(typeof(RoomAreaMaskType)))
                {
                    if (mask == RoomAreaMaskType.None)
                        continue;

                    nativeRoom.SetAreaMask(mask, _maskCache[mask].Image);
                }

                nativeRoom.SaveToFile(_loadedRoom.FileName);
            }
        }
        #endregion // Upgrade Crm Format To Open Format
    }
}
