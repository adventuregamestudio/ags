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
    class GuiComponent : BaseComponentWithFolders<GUI, GUIFolder>
    {
        private const string GUI_FILE_FILTER = "AGS exported GUIs (*.guf)|*.guf";

        private const string GUIS_COMMAND_ID = "GUI";
        private const string COMMAND_NEW_GUI = "NewGUI";
        private const string COMMAND_NEW_TEXTWINDOW = "NewTextWindow";
        private const string COMMAND_IMPORT_GUI = "ImportGUI";
        private const string COMMAND_DELETE_GUI = "DeleteGUI";
        private const string COMMAND_EXPORT_GUI = "ExportGUI";
        private const string COMMAND_FIND_ALL_USAGES = "FindAllUsages";
        private const string ICON_KEY = "GUIsIcon";
        
        internal const string MODE_SELECT_CONTROLS = "SelectControls";
        internal const string MODE_ADD_BUTTON = "AddButton";
        internal const string MODE_ADD_LABEL = "AddLabel";
        internal const string MODE_ADD_TEXTBOX = "AddTextbox";
        internal const string MODE_ADD_LISTBOX = "AddListbox";
        internal const string MODE_ADD_SLIDER = "AddSlider";
        internal const string MODE_ADD_INVENTORY = "AddInventory";

        private GUI _guiRightClicked;
        private Dictionary<GUI, ContentDocument> _documents;

        public GuiComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, GUIS_COMMAND_ID)
        {
            _documents = new Dictionary<GUI, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("icongui.ico"));
            _guiController.RegisterIcon("GUIIcon", Resources.ResourceManager.GetIcon("icongui-item.ico"));
            _guiController.RegisterIcon("SelectGUIIcon", Resources.ResourceManager.GetIcon("cursor.ico"));
            _guiController.RegisterIcon("GUIButtonIcon", Resources.ResourceManager.GetIcon("guis_button.ico"));
            _guiController.RegisterIcon("GUIInvWindowIcon", Resources.ResourceManager.GetIcon("guis_inventory-window.ico"));
            _guiController.RegisterIcon("GUILabelIcon", Resources.ResourceManager.GetIcon("guis_label.ico"));
            _guiController.RegisterIcon("GUIListBoxIcon", Resources.ResourceManager.GetIcon("guis_listbox.ico"));
            _guiController.RegisterIcon("GUISliderIcon", Resources.ResourceManager.GetIcon("guis_slider.ico"));
            _guiController.RegisterIcon("GUITextBoxIcon", Resources.ResourceManager.GetIcon("guis_textbox.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "GUIs", ICON_KEY);

            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.GUIs; }
        }

        protected override void ItemCommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_GUI)
            {
                AddNewGUI(new NormalGUI());
            }
            else if (controlID == COMMAND_NEW_TEXTWINDOW)
            {
                AddNewGUI(new TextWindowGUI());
            }
            else if (controlID == COMMAND_IMPORT_GUI)
            {
                string fileName = _guiController.ShowOpenFileDialog("Import GUI...", GUI_FILE_FILTER);
                if (fileName != null)
                {
                    try
                    {
                        GUI newGui = ImportExport.ImportGUIFromFile(fileName, _agsEditor.CurrentGame);
                        newGui.ID = _agsEditor.CurrentGame.RootGUIFolder.GetAllItemsCount();
                        AddSingleItem(newGui);                        
                    }
                    catch (Exception ex)
                    {
                        _guiController.ShowMessage("There was an error importing the GUI. The error was: " + ex.Message, MessageBoxIcon.Warning);
                    }
                }
            }
            else if (controlID == COMMAND_FIND_ALL_USAGES)
            {
                FindAllUsages findAllUsage = new FindAllUsages(null, null, null, _agsEditor);
                findAllUsage.Find(null, _guiRightClicked.Name);
            }
            else if (controlID == COMMAND_EXPORT_GUI)
            {
                string fileName = _guiController.ShowSaveFileDialog("Export GUI as...", GUI_FILE_FILTER);
                if (fileName != null)
                {
                    try
                    {
                        ImportExport.ExportGUIToFile(_guiRightClicked, fileName, _agsEditor.CurrentGame);
                    }
                    catch (Exception ex)
                    {
                        _guiController.ShowMessage("There was an error exporting the GUI. The error was: " + ex.Message, MessageBoxIcon.Warning);
                    }
                }
            }
            else if (controlID == COMMAND_DELETE_GUI)
            {
                if (MessageBox.Show("Are you sure you want to delete this GUI? Doing so could break any scripts that refer to GUIs by number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    DeleteSingleItem(_guiRightClicked);
                }
            }
            else if ((!controlID.StartsWith(NODE_ID_PREFIX_FOLDER)) &&
                     (controlID != TOP_LEVEL_COMMAND_ID))
            {
                GUI chosenGui = _agsEditor.CurrentGame.RootGUIFolder.FindGUIByID
                    (Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length)), true);
                ShowOrAddPane(chosenGui);
            }
        }

        private void DeleteGUI(GUI guiToDelete)
        {
            int removingID = guiToDelete.ID;
            foreach (GUI gui in _agsEditor.CurrentGame.RootGUIFolder.AllItemsFlat)
            {
                if (gui.ID > removingID)
                {
                    gui.ID--;
                }
            }
            ContentDocument document;
            if (_documents.TryGetValue(guiToDelete, out document))
            {
                _guiController.RemovePaneIfExists(document);
                _documents.Remove(guiToDelete);
            }
        }

        protected override void DeleteResourcesUsedByItem(GUI item)
        {
            DeleteGUI(item);
        }

		private void ShowOrAddPane(GUI chosenGui)
		{
            ContentDocument document;
			if (!_documents.TryGetValue(chosenGui, out document) 
                || document.Control.IsDisposed)
			{
				List<MenuCommand> toolbarIcons = CreateToolbarIcons();
				GUIEditor editor = new GUIEditor(chosenGui, toolbarIcons);
				editor.OnControlsChanged += new GUIEditor.ControlsChanged(_guiEditor_OnControlsChanged);
				editor.OnGuiNameChanged += new GUIEditor.GuiNameChanged(_guiEditor_OnGuiNameChanged);
                document = new ContentDocument(editor, chosenGui.WindowTitle,
                    this, ICON_KEY, ConstructPropertyObjectList(chosenGui));
                _documents[chosenGui] = document;
                document.SelectedPropertyGridObject = chosenGui;
				if (chosenGui is NormalGUI)
				{
                    document.ToolbarCommands = toolbarIcons;
				}
			}
            document.TreeNodeID = GetNodeID(chosenGui);
            _guiController.AddOrShowPane(document);
			_guiController.ShowCuppit("The GUI Editor is where you set up the GUIs in your game. Use the buttons in the toolbar to add controls, and the property grid on the right to edit the selected control's properties.", "GUI Editor introduction");
		}

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            GUIEditor editor = (GUIEditor)_guiController.ActivePane.Control;
            _guiEditor_OnControlsChanged(editor.GuiToEdit);

			if (propertyName == "Name")
			{
				foreach (ContentDocument doc in _documents.Values)
				{
					doc.Name = ((GUIEditor)doc.Control).GuiToEdit.WindowTitle;
				}
			}
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_GUI, "New GUI", null));
            menu.Add(new MenuCommand(COMMAND_NEW_TEXTWINDOW, "New Text Window GUI", null));
            menu.Add(new MenuCommand(COMMAND_IMPORT_GUI, "Import GUI...", null));
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            if ((controlID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!IsFolderNode(controlID)))
            {
                int guiID = Convert.ToInt32(controlID.Substring(ITEM_COMMAND_PREFIX.Length));
                GUI gui = _agsEditor.CurrentGame.RootGUIFolder.FindGUIByID(guiID, true);
                _guiRightClicked = gui;
                menu.Add(new MenuCommand(COMMAND_DELETE_GUI, "Delete " + gui.Name, null));
                menu.Add(new MenuCommand(COMMAND_EXPORT_GUI, "Export GUI...", null));
                menu.Add(new MenuCommand(COMMAND_FIND_ALL_USAGES, "Find All Usages of " + gui.Name, null));
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

        public override void GameSettingsChanged()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                ((GUIEditor)doc.Control).Invalidate();
            }
        }

        private List<MenuCommand> CreateToolbarIcons()
        {
            List<MenuCommand> toolbarIcons = new List<MenuCommand>();
            toolbarIcons.Add(new MenuCommand(MODE_SELECT_CONTROLS, "Select controls", "SelectGUIIcon"));
            toolbarIcons.Add(new MenuCommand(MODE_ADD_BUTTON, "Add GUI Button", "GUIButtonIcon"));
            toolbarIcons.Add(new MenuCommand(MODE_ADD_LABEL, "Add GUI Label", "GUILabelIcon"));
            toolbarIcons.Add(new MenuCommand(MODE_ADD_TEXTBOX, "Add GUI Text Box", "GUITextBoxIcon"));
            toolbarIcons.Add(new MenuCommand(MODE_ADD_LISTBOX, "Add GUI List Box", "GUIListBoxIcon"));
            toolbarIcons.Add(new MenuCommand(MODE_ADD_SLIDER, "Add GUI Slider", "GUISliderIcon"));
            toolbarIcons.Add(new MenuCommand(MODE_ADD_INVENTORY, "Add Inventory Window", "GUIInvWindowIcon"));
            toolbarIcons[0].Checked = true;
            return toolbarIcons;
        }

        private void AddNewGUI(GUI newGui)
        {            
            newGui.ID = _agsEditor.CurrentGame.RootGUIFolder.GetAllItemsCount();
            newGui.Name = _agsEditor.GetFirstAvailableScriptName("gGui");
            string newNodeId = AddSingleItem(newGui);
            _guiController.ProjectTree.SelectNode(this, newNodeId);
			ShowOrAddPane(newGui);
        }
        
        private Dictionary<string, object> ConstructPropertyObjectList(GUI forGui)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(forGui.PropertyGridTitle, forGui);
            foreach (GUIControl control in forGui.Controls)
            {
                list.Add(control.Name + " (" + control.ControlType + "; ID " + control.ID + ")", control);
            }
            return list;
        }

        private void _guiEditor_OnControlsChanged(GUI editingGui)
        {
            _guiController.SetPropertyGridObjectList(ConstructPropertyObjectList(editingGui));
        }

        private void _guiEditor_OnGuiNameChanged()
        {
            RePopulateTreeView();
        }

        private string GetNodeID(GUI item)
        {
            return ITEM_COMMAND_PREFIX + item.ID;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(GUI item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item), item.ID.ToString() + ": " + item.Name, "GUIIcon");
            return treeItem;
        }

        protected override bool CanFolderBeDeleted(GUIFolder folder)
        {
            return true;
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its GUIs?" + Environment.NewLine + Environment.NewLine + "If any of the GUIs are referenced in code by their number it could cause crashes in the game.";
        }

        protected override GUIFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootGUIFolder;
        }

    }
}
