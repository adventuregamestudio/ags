using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class CharactersComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Characters";
        private const string COMMAND_NEW_ITEM = "NewCharacter";
        private const string COMMAND_IMPORT = "ImportCharacter";
        private const string COMMAND_DELETE_ITEM = "DeleteCharacter";
        private const string COMMAND_EXPORT = "ExportCharacter";

        private const string CHARACTER_EXPORT_FILE_FILTER = "AGS 3.1+ exported characters (*.chr)|*.chr|AGS 2.72/3.0 exported characters (*.cha)|*.cha";
        private const string CHARACTER_IMPORT_FILE_FILTER = "AGS exported characters (*.chr; *.cha)|*.chr;*.cha|AGS 3.1+ exported characters (*.chr)|*.chr|AGS 2.72/3.0 exported characters (*.cha)|*.cha";
        private const string NEW_CHARACTER_FILE_EXTENSION = ".chr";

        private Dictionary<Character, ContentDocument> _documents;
        private Character _itemRightClicked = null;

        public CharactersComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _documents = new Dictionary<Character, ContentDocument>();
            _guiController.RegisterIcon("CharactersIcon", Resources.ResourceManager.GetIcon("charactr.ico"));
            _guiController.RegisterIcon("CharacterIcon", Resources.ResourceManager.GetIcon("charactr-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Characters", "CharactersIcon");
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Characters; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                IList<Character> items = _agsEditor.CurrentGame.Characters;
                Character newItem = new Character();
                newItem.ID = items.Count;
                newItem.ScriptName = _agsEditor.GetFirstAvailableScriptName("cChar");
                newItem.RealName = "New character";
                items.Add(newItem);
                _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
                _guiController.ProjectTree.AddTreeLeaf(this, "Chr" + newItem.ID, newItem.ID.ToString() + ": " + newItem.ScriptName, "CharacterIcon");
                _guiController.ProjectTree.SelectNode(this, "Chr" + newItem.ID);
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
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this character? Doing so could break any scripts that refer to characters by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    int removingID = _itemRightClicked.ID;
                    foreach (Character item in _agsEditor.CurrentGame.Characters)
                    {
                        if (item.ID > removingID)
                        {
                            item.ID--;
                        }
                    }
                    if (_documents.ContainsKey(_itemRightClicked))
                    {
                        _guiController.RemovePaneIfExists(_documents[_itemRightClicked]);
                        _documents.Remove(_itemRightClicked);
                    }
                    _agsEditor.CurrentGame.Characters.Remove(_itemRightClicked);
                    RePopulateTreeView();
                }
            }
            else if (controlID != TOP_LEVEL_COMMAND_ID)
            {
                Character chosenItem = _agsEditor.CurrentGame.Characters[Convert.ToInt32(controlID.Substring(3))];
				ShowOrAddPane(chosenItem);
			}
        }

		private void ShowOrAddPane(Character chosenItem)
		{
			if (!_documents.ContainsKey(chosenItem))
			{
				_documents.Add(chosenItem, new ContentDocument(new CharacterEditor(chosenItem), chosenItem.WindowTitle, this, ConstructPropertyObjectList(chosenItem)));
				_documents[chosenItem].SelectedPropertyGridObject = chosenItem;
			}
			_guiController.AddOrShowPane(_documents[chosenItem]);
			_guiController.ShowCuppit("Characters can move around from room to room within the game, and can take part in conversations. The Player Character is the one that the player is controlling.", "Characters introduction");
		}

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            Character itemBeingEdited = ((CharacterEditor)_guiController.ActivePane.Control).ItemToEdit;

            if (propertyName == "ScriptName")
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
                    // Refresh tree, property grid and open windows
                    RePopulateTreeView();
                    _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(itemBeingEdited));

                    foreach (ContentDocument doc in _documents.Values)
                    {
                        doc.Name = ((CharacterEditor)doc.Control).ItemToEdit.WindowTitle;
                    }
                }
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New character", null));
                menu.Add(new MenuCommand(COMMAND_IMPORT, "Import character...", null));
            }
            else
            {
                int charID = Convert.ToInt32(controlID.Substring(3));
                _itemRightClicked = _agsEditor.CurrentGame.Characters[charID];
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this character", null));
                menu.Add(new MenuCommand(COMMAND_EXPORT, "Export character...", null));
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
                
                character.ID = _agsEditor.CurrentGame.Characters.Count;
                _agsEditor.CurrentGame.Characters.Add(character);
            }
            catch (ApplicationException ex)
            {
                _guiController.ShowMessage("An error occurred importing the character file. The error was: " + Environment.NewLine + Environment.NewLine + ex.Message, MessageBoxIcon.Warning);
            }
            RePopulateTreeView();
        }

        private void RePopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            foreach (Character item in _agsEditor.CurrentGame.Characters)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, "Chr" + item.ID, item.ID.ToString() + ": " + item.ScriptName, "CharacterIcon");
            }

            if (_documents.ContainsValue(_guiController.ActivePane))
            {
                CharacterEditor editor = (CharacterEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, "Chr" + editor.ItemToEdit.ID);
            }
            else if (_agsEditor.CurrentGame.Characters.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "Chr0");
            }
        }

        private Dictionary<string, object> ConstructPropertyObjectList(Character item)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(item.ScriptName + " (Character " + item.ID + ")", item);
            return list;
        }
    }
}
