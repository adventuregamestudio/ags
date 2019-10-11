using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;
using AGS.Editor.TextProcessing;

namespace AGS.Editor.Components
{
    class CharactersComponent : BaseComponentWithFolders<Character, CharacterFolder>
    {
        private const string CHARACTERS_COMMAND_ID = "Characters";
        private const string COMMAND_NEW_ITEM = "NewCharacter";
        private const string COMMAND_IMPORT = "ImportCharacter";
        private const string COMMAND_DELETE_ITEM = "DeleteCharacter";
        private const string COMMAND_EXPORT = "ExportCharacter";
        private const string COMMAND_CHANGE_ID = "ChangeCharacterID";
        private const string COMMAND_FIND_ALL_USAGES = "FindAllUsages";
        private const string ICON_KEY = "CharactersIcon";
        
        private const string CHARACTER_EXPORT_FILE_FILTER = "AGS 3.1+ exported characters (*.chr)|*.chr|AGS 2.72/3.0 exported characters (*.cha)|*.cha";
        private const string CHARACTER_IMPORT_FILE_FILTER = "AGS exported characters (*.chr; *.cha)|*.chr;*.cha|AGS 3.1+ exported characters (*.chr)|*.chr|AGS 2.72/3.0 exported characters (*.cha)|*.cha";
        private const string NEW_CHARACTER_FILE_EXTENSION = ".chr";

        private Dictionary<Character, ContentDocument> _documents;
        private Character _itemRightClicked = null;

        public event EventHandler<CharacterIDChangedEventArgs> OnCharacterIDChanged;
        public event EventHandler<CharacterRoomChangedEventArgs> OnCharacterRoomChanged;

        public CharactersComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, CHARACTERS_COMMAND_ID)
        {
            _documents = new Dictionary<Character, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("charactr.ico"));
            _guiController.RegisterIcon("CharacterIcon", Resources.ResourceManager.GetIcon("charactr-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Characters", ICON_KEY);
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Characters; }
        }

        protected override void ItemCommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {                
                Character newItem = new Character();
                newItem.ID = _agsEditor.CurrentGame.RootCharacterFolder.GetAllItemsCount();
                newItem.ScriptName = _agsEditor.GetFirstAvailableScriptName("cChar");
                newItem.RealName = "New character";
                newItem.StartingRoom = -1;
                string newNodeID = AddSingleItem(newItem);
                _guiController.ProjectTree.SelectNode(this, newNodeID);
				ShowOrAddPane(newItem);
            }
            else if (controlID == COMMAND_IMPORT)
            {
                string fileName = _guiController.ShowOpenFileDialog("Select character to import...", CHARACTER_IMPORT_FILE_FILTER);
                if (fileName != null)
                {
                    ImportCharacter(fileName);
                }
            }
            else if (controlID == COMMAND_EXPORT)
            {
                string fileName = _guiController.ShowSaveFileDialog("Export character as...", CHARACTER_EXPORT_FILE_FILTER);
                if (fileName != null)
                {
                    ExportCharacter(_itemRightClicked, fileName);
                }
            }
            else if (controlID == COMMAND_FIND_ALL_USAGES)
            {
                FindAllUsages findAllUsages = new FindAllUsages(null, null, null, _agsEditor);
                findAllUsages.Find(null, _itemRightClicked.ScriptName);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this character? Doing so could break any scripts that refer to characters by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    Character c = _itemRightClicked;
                    // For a lack of better solution at the moment, pretend that character leaves current room before being deleted
                    int oldRoom = c.StartingRoom;
                    c.StartingRoom = -1;
                    OnCharacterRoomChanged?.Invoke(this, new CharacterRoomChangedEventArgs(c, oldRoom));
                    DeleteSingleItem(_itemRightClicked);
                }
            }
            else if (controlID == COMMAND_CHANGE_ID)
            {
                int oldNumber = _itemRightClicked.ID;
                int newNumber = Factory.GUIController.ShowChangeObjectIDDialog("Character", oldNumber, 0, _items.Count - 1);
                if (newNumber < 0)
                    return;
                foreach (var obj in _items)
                {
                    if (obj.Value.ID == newNumber)
                    {
                        obj.Value.ID = oldNumber;
                        break;
                    }
                }
                _itemRightClicked.ID = newNumber;
                OnItemIDChanged(_itemRightClicked);
            }
            else if ((!controlID.StartsWith(NODE_ID_PREFIX_FOLDER)) &&
                     (controlID != TOP_LEVEL_COMMAND_ID))
            {
                Character chosenItem = _items[controlID];
                ShowOrAddPane(chosenItem);
            }            
        }

        private void DeleteCharacter(Character character)
        {
            int removingID = character.ID;
            foreach (Character item in _agsEditor.CurrentGame.RootCharacterFolder.AllItemsFlat)
            {
                if (item.ID > removingID)
                {
                    item.ID--;
                    OnCharacterIDChanged?.Invoke(this, new CharacterIDChangedEventArgs(item, item.ID + 1));
                }
            }

            ContentDocument document;
            if (_documents.TryGetValue(character, out document))
            {
                _guiController.RemovePaneIfExists(document);
                _documents.Remove(character);
            }            
        }

		private void ShowOrAddPane(Character chosenItem)
		{
            ContentDocument document;
			if (!_documents.TryGetValue(chosenItem, out document)
                || document.Control.IsDisposed)
			{
                document = new ContentDocument(new CharacterEditor(chosenItem),
                    chosenItem.WindowTitle, this, ICON_KEY,
                    ConstructPropertyObjectList(chosenItem));
				_documents[chosenItem] = document;
				document.SelectedPropertyGridObject = chosenItem;
			}
            document.TreeNodeID = GetNodeID(chosenItem);
			_guiController.AddOrShowPane(document);
		}

        private void OnItemIDChanged(Character item)
        {
            // Refresh tree, property grid and open windows
            RePopulateTreeView();
            _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(item));

            foreach (ContentDocument doc in _documents.Values)
            {
                doc.Name = ((CharacterEditor)doc.Control).ItemToEdit.WindowTitle;
            }
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            Character itemBeingEdited = ((CharacterEditor)_guiController.ActivePane.Control).ItemToEdit;

            if (propertyName == Character.PROPERTY_NAME_SCRIPTNAME)
            {
                bool nameInUse = _agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.ScriptName, itemBeingEdited);
                if (itemBeingEdited.ScriptName.StartsWith("c") && itemBeingEdited.ScriptName.Length > 1)
                {
                    nameInUse |= _agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.ScriptName.Substring(1).ToUpper(), itemBeingEdited);
                }
                if (nameInUse)
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIcon.Warning);
                    itemBeingEdited.ScriptName = (string)oldValue;
                }
                else
                {
                    OnItemIDChanged(itemBeingEdited);
                }
            }
            else if (propertyName == Character.PROPERTY_NAME_STARTINGROOM)
            {
                if (OnCharacterRoomChanged != null)
                {
                    int oldRoom = (int)oldValue;
                    OnCharacterRoomChanged(this, new CharacterRoomChangedEventArgs(itemBeingEdited, oldRoom));
                }
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            if ((controlID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!IsFolderNode(controlID)))
            {
                int charID = Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length));
                _itemRightClicked = _agsEditor.CurrentGame.RootCharacterFolder.FindCharacterByID(charID, true);
                menu.Add(new MenuCommand(COMMAND_CHANGE_ID, "Change character ID", null));
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this character", null));
                menu.Add(new MenuCommand(COMMAND_EXPORT, "Export character...", null));
                menu.Add(new MenuCommand(COMMAND_FIND_ALL_USAGES, "Find All Usages of " + _itemRightClicked.ScriptName, null));
            }
            return menu;
        }

        public override void RefreshDataFromGame()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                _guiController.RemovePaneIfExists(doc);
                doc.Dispose();
            }
            _documents.Clear();

            RePopulateTreeView();
        }

        private void ExportCharacter(Character character, string fileName)
        {
            try
            {
                if (fileName.ToLower().EndsWith(NEW_CHARACTER_FILE_EXTENSION))
                {
                    ImportExport.ExportCharacterNewFormat(character, fileName, _agsEditor.CurrentGame);
                }
                else
                {
                    ImportExport.ExportCharacter272(character, fileName, _agsEditor.CurrentGame);
                }
            }
            catch (ApplicationException ex)
            {
                _guiController.ShowMessage("An error occurred exporting the character file. The error was: " + ex.Message, MessageBoxIcon.Warning);
            }
        }

        private void ImportCharacter(string fileName)
        {
            try
            {
                Character character;

                if (fileName.ToLower().EndsWith(NEW_CHARACTER_FILE_EXTENSION))
                {
                    character = ImportExport.ImportCharacterNew(fileName, _agsEditor.CurrentGame);
                }
                else
                {
                    character = ImportExport.ImportCharacter272(fileName, _agsEditor.CurrentGame);
                }

                character.ID = _agsEditor.CurrentGame.RootCharacterFolder.GetAllItemsCount();
                AddSingleItem(character);
                // Pretend that character has just changed into the new room
                OnCharacterRoomChanged?.Invoke(this, new CharacterRoomChangedEventArgs(character, -1));
            }
            catch (ApplicationException ex)
            {
                _guiController.ShowMessage("An error occurred importing the character file. The error was: " + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
            }
            RePopulateTreeView();
        }        

        private Dictionary<string, object> ConstructPropertyObjectList(Character item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.ScriptName + " (Character " + item.ID + ")", item);
            return list;
        }

        protected override CharacterFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootCharacterFolder;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(Character item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item), item.ID.ToString() + ": " + item.ScriptName, "CharacterIcon");            
            return treeItem;
        }

        private string GetNodeID(Character character)
        {
            return ITEM_COMMAND_PREFIX + character.ID;
        }

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New character", null));
            menu.Add(new MenuCommand(COMMAND_IMPORT, "Import character...", null));  
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            // No more commands in this menu
        }

        protected override bool CanFolderBeDeleted(CharacterFolder folder)
        {
            return true;
        }

        protected override void DeleteResourcesUsedByItem(Character item)
        {
            DeleteCharacter(item);
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its characters?" + Environment.NewLine + Environment.NewLine + "If any of the characters are referenced in code by their number it could cause crashes in the game.";
        }
    }
}
