using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class GuiComponent : BaseComponent
    {
        private const string GUI_FILE_FILTER = "AGS exported GUIs (*.guf)|*.guf";

        private const string TOP_LEVEL_COMMAND_ID = "GUI";
        private const string COMMAND_NEW_GUI = "NewGUI";
        private const string COMMAND_NEW_TEXTWINDOW = "NewTextWindow";
        private const string COMMAND_IMPORT_GUI = "ImportGUI";
        private const string COMMAND_DELETE_GUI = "DeleteGUI";
        private const string COMMAND_EXPORT_GUI = "ExportGUI";

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
            : base(guiController, agsEditor)
        {
            _documents = new Dictionary<GUI, ContentDocument>();
            _guiController.RegisterIcon("GUIsIcon", Resources.ResourceManager.GetIcon("icongui.ico"));
            _guiController.RegisterIcon("GUIIcon", Resources.ResourceManager.GetIcon("icongui-item.ico"));
            _guiController.RegisterIcon("SelectGUIIcon", Resources.ResourceManager.GetIcon("cursor.ico"));
            _guiController.RegisterIcon("GUIButtonIcon", Resources.ResourceManager.GetIcon("guis_button.ico"));
            _guiController.RegisterIcon("GUIInvWindowIcon", Resources.ResourceManager.GetIcon("guis_inventory-window.ico"));
            _guiController.RegisterIcon("GUILabelIcon", Resources.ResourceManager.GetIcon("guis_label.ico"));
            _guiController.RegisterIcon("GUIListBoxIcon", Resources.ResourceManager.GetIcon("guis_listbox.ico"));
            _guiController.RegisterIcon("GUISliderIcon", Resources.ResourceManager.GetIcon("guis_slider.ico"));
            _guiController.RegisterIcon("GUITextBoxIcon", Resources.ResourceManager.GetIcon("guis_textbox.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "GUIs", "GUIsIcon");

            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.GUIs; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                // click on the parent "GUI" node
            }
            else if (controlID == COMMAND_NEW_GUI)
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
                        newGui.ID = _agsEditor.CurrentGame.GUIs.Count;
                        _agsEditor.CurrentGame.GUIs.Add(newGui);
                        RePopulateTreeView();
                    }
                    catch (Exception ex)
                    {
                        _guiController.ShowMessage("There was an error importing the GUI. The error was: " + ex.Message, MessageBoxIcon.Warning);
                    }
                }
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
                    int removingID = _guiRightClicked.ID;
                    foreach (GUI gui in _agsEditor.CurrentGame.GUIs)
                    {
                        if (gui.ID > removingID)
                        {
                            gui.ID--;
                        }
                    }
                    if (_documents.ContainsKey(_guiRightClicked))
                    {
                        _guiController.RemovePaneIfExists(_documents[_guiRightClicked]);
                        _documents.Remove(_guiRightClicked);
                    }
                    _agsEditor.CurrentGame.GUIs.Remove(_guiRightClicked);
                    RePopulateTreeView();
                }
            }
            else
            {
                GUI chosenGui = _agsEditor.CurrentGame.GUIs[Convert.ToInt32(controlID.Substring(3))];
				ShowOrAddPane(chosenGui);
			}
        }

		private void ShowOrAddPane(GUI chosenGui)
		{
			if (!_documents.ContainsKey(chosenGui))
			{
				List<MenuCommand> toolbarIcons = CreateToolbarIcons();
				GUIEditor editor = new GUIEditor(chosenGui, toolbarIcons);
				editor.OnControlsChanged += new GUIEditor.ControlsChanged(_guiEditor_OnControlsChanged);
				editor.OnGuiNameChanged += new GUIEditor.GuiNameChanged(_guiEditor_OnGuiNameChanged);
				_documents.Add(chosenGui, new ContentDocument(editor, chosenGui.WindowTitle, this, ConstructPropertyObjectList(chosenGui)));
				_documents[chosenGui].SelectedPropertyGridObject = chosenGui;
				if (chosenGui is NormalGUI)
				{
					_documents[chosenGui].ToolbarCommands = toolbarIcons;
				}
			}
			_guiController.AddOrShowPane(_documents[chosenGui]);
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

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_GUI, "New GUI", null));
                menu.Add(new MenuCommand(COMMAND_NEW_TEXTWINDOW, "New Text Window GUI", null));
                menu.Add(new MenuCommand(COMMAND_IMPORT_GUI, "Import GUI...", null));
            }
            else
            {
                int guiID = Convert.ToInt32(controlID.Substring(3));
                GUI gui = _agsEditor.CurrentGame.GUIs[guiID];
                _guiRightClicked = gui;
                menu.Add(new MenuCommand(COMMAND_DELETE_GUI, "Delete " + gui.Name, null));
                menu.Add(new MenuCommand(COMMAND_EXPORT_GUI, "Export GUI...", null));
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
            IList<GUI> guis = _agsEditor.CurrentGame.GUIs;
            newGui.ID = guis.Count;
            newGui.Name = _agsEditor.GetFirstAvailableScriptName("gGui");
            guis.Add(newGui);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.AddTreeLeaf(this, "GUI" + newGui.ID, newGui.ID.ToString() + ": " + newGui.Name, "GUIIcon");
            _guiController.ProjectTree.SelectNode(this, "GUI" + newGui.ID);
			ShowOrAddPane(newGui);
        }

        private void RePopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            foreach (GUI gui in _agsEditor.CurrentGame.GUIs)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, "GUI" + gui.ID, gui.ID.ToString() + ": " + gui.Name, "GUIIcon");
            }
            if (_documents.ContainsValue(_guiController.ActivePane))
            {
                GUIEditor editor = (GUIEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, "GUI" + editor.GuiToEdit.ID);
            }
            else if (_agsEditor.CurrentGame.GUIs.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "GUI0");
            }
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
    }
}
