using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
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
        private const int    GUI_DEFAULT_WIDTH_MAX = 640;
        private const int    GUI_DEFAULT_HEIGHT_MAX = 480;

        private const string GUIS_COMMAND_ID = "GUI";
        private const string COMMAND_NEW_GUI = "NewGUI";
        private const string COMMAND_NEW_TEXTWINDOW = "NewTextWindow";
        private const string COMMAND_IMPORT_GUI = "ImportGUI";
        private const string COMMAND_DELETE_GUI = "DeleteGUI";
        private const string COMMAND_EXPORT_GUI = "ExportGUI";
        private const string COMMAND_CHANGE_ID = "ChangeGUIID";
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
            _agsEditor.TestGameScripts += ScanAndReportMissingEventHandlers;

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
                Size gameRes = _agsEditor.CurrentGame.Settings.CustomResolution;
                GUI newGUI = new NormalGUI(Math.Min(gameRes.Width, GUI_DEFAULT_WIDTH_MAX), Math.Min(gameRes.Height, GUI_DEFAULT_HEIGHT_MAX));
                // Default GUI colors are set as palette indexes, remap them to proper colors
                Tasks.RemapGUIColours(newGUI, _agsEditor.CurrentGame, GameColorDepth.Palette);
                AddNewGUI(newGUI);
                _agsEditor.CurrentGame.NotifyClientsGUIAddedOrRemoved(newGUI);
            }
            else if (controlID == COMMAND_NEW_TEXTWINDOW)
            {
                GUI newGUI = new TextWindowGUI();
                AddNewGUI(newGUI);
                _agsEditor.CurrentGame.NotifyClientsGUIAddedOrRemoved(newGUI);
            }
            else if (controlID == COMMAND_IMPORT_GUI)
            {
                string fileName = _guiController.ShowOpenFileDialog("Import GUI...", GUI_FILE_FILTER);
                if (fileName != null)
                {
                    try
                    {
                        GUI newGUI = ImportExport.ImportGUIFromFile(fileName, _agsEditor.CurrentGame);
                        newGUI.ID = _agsEditor.CurrentGame.RootGUIFolder.GetAllItemsCount();
                        AddSingleItem(newGUI);
                        _agsEditor.CurrentGame.NotifyClientsGUIAddedOrRemoved(newGUI);
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
                    _agsEditor.CurrentGame.NotifyClientsGUIAddedOrRemoved(_guiRightClicked);
                    DeleteSingleItem(_guiRightClicked);
                }
            }
            else if (controlID == COMMAND_CHANGE_ID)
            {
                int oldNumber = _guiRightClicked.ID;
                int newNumber = Factory.GUIController.ShowChangeObjectIDDialog("GUI", oldNumber, 0, _items.Count - 1);
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
                _guiRightClicked.ID = newNumber;
                GetFlatList().Swap(oldNumber, newNumber);
                OnItemIDOrNameChanged(_guiRightClicked, false);
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
                    this, ICON_KEY, GUIEditor.ConstructPropertyObjectList(chosenGui));
                _documents[chosenGui] = document;
                document.SelectedPropertyGridObject = chosenGui;
				if (chosenGui is NormalGUI)
				{
                    document.ToolbarCommands = toolbarIcons;
				}
			}
            document.TreeNodeID = GetNodeID(chosenGui);
            _guiController.AddOrShowPane(document);
		}

        public override IList<string> GetManagedScriptElements()
        {
            return new string[] { "GUI", "Label", "Button", "Slider", "ListBox", "TextBox", "InvWindow" };
        }

        public override void ShowItemPaneByName(string name)
        {
            IList<GUI> guis = GetFlatList();
            foreach (GUI g in guis)
            {
                if (g.Name == name)
                {
                    _guiController.ProjectTree.SelectNode(this, GetNodeID(g));
                    ShowOrAddPane(g);
                    return;
                }
                
                foreach(GUIControl gctrl in g.Controls)
                {
                    if(gctrl.Name == name)
                    {
                        _guiController.ProjectTree.SelectNode(this, GetNodeID(g));
                        ShowOrAddPane(g);
                        Factory.GUIController.SetPropertyGridObject(gctrl);
                        return;
                    }
                }
            }
        }

        private void OnItemIDOrNameChanged(GUI item, bool name_only)
        {
            // Refresh tree, property grid and open windows
            if (name_only)
                ChangeItemLabel(GetNodeID(item), GetNodeLabel(item));
            else
                RePopulateTreeView(GetNodeID(item)); // currently this is the only way to update tree item ids

            foreach (ContentDocument doc in _documents.Values)
            {
                var docItem = ((GUIEditor)doc.Control).GuiToEdit;
                doc.Name = docItem.WindowTitle;
                _guiController.SetPropertyGridObjectList(GUIEditor.ConstructPropertyObjectList(docItem), doc, docItem);
            }
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            GUIEditor editor = (GUIEditor)_guiController.ActivePane.Control;
            GUI itemBeingEdited = editor.GuiToEdit;
            _guiEditor_OnControlsChanged(editor.GuiToEdit);

 			// FIXME: this does not distinguish GUI and GUIControl properties!
 			if (propertyName == "Name")
			{
                OnItemIDOrNameChanged(itemBeingEdited, true);
			}
        }

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_GUI, "New GUI", null));
            menu.Add(new MenuCommand(COMMAND_NEW_TEXTWINDOW, "New Text Window GUI", null));
            menu.Add(new MenuCommand(COMMAND_IMPORT_GUI, "Import GUI...", null));
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            // No more commands in this menu
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
                menu.Add(new MenuCommand(COMMAND_CHANGE_ID, "Change GUI ID", null));
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

        private void _guiEditor_OnControlsChanged(GUI editingGui)
        {
        }

        private void _guiEditor_OnGuiNameChanged()
        {
            GUI itemBeingEdited = ((GUIEditor)_guiController.ActivePane.Control).GuiToEdit;
            RePopulateTreeView(GetNodeID(itemBeingEdited));
        }

        private string GetNodeID(GUI item)
        {
            return ITEM_COMMAND_PREFIX + item.ID;
        }

        private string GetNodeLabel(GUI item)
        {
            return item.ID.ToString() + ": " + item.Name;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(GUI item)
        {
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf
                (this, GetNodeID(item), GetNodeLabel(item), "GUIIcon");
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

        protected override IList<GUI> GetFlatList()
        {
            return _agsEditor.CurrentGame.GUIFlatList;
        }

        /// <summary>
        /// Helper class for use when scanning for event handlers
        /// </summary>
        private class GUIObjectWithEvents
        {
            public NormalGUI GUI;
            public GUIControl Control;

            public GUIObjectWithEvents(NormalGUI gui) { GUI = gui; }
            public GUIObjectWithEvents(GUIControl control) { Control = control; }
        }

        private class GUIEventReference
        {
            public GUIObjectWithEvents GUIObject;
            public string EventName;
            public string FunctionName;

            public GUIEventReference(GUIObjectWithEvents obj, string evtName, string fnName)
            {
                GUIObject = obj;
                EventName = evtName;
                FunctionName = fnName;
            }
        }

        private void ScanAndReportMissingEventHandlers(GenericMessagesArgs args)
        {
            var errors = args.Messages;
            foreach (GUI gui in _agsEditor.CurrentGame.GUIs)
            {
                NormalGUI ngui = gui as NormalGUI;
                if (ngui == null)
                    continue;

                // Gather function names from the GUI and all of their controls,
                // in order to test missing functions in a single batch.
                // TODO: the following code would be simpler if there was an indexed Events table in each GUI and control class;
                // see also: how Interactions class is done for Characters etc.
                List<GUIEventReference> objectEvents = new List<GUIEventReference>();
                objectEvents.Add(new GUIEventReference(new GUIObjectWithEvents(ngui), "OnClick", ngui.OnClick));

                foreach (var control in ngui.Controls)
                {
                    GUIObjectWithEvents obj = new GUIObjectWithEvents(control);
                    if (control is GUIButton)
                    {
                        objectEvents.Add(new GUIEventReference(obj, "OnClick", (control as GUIButton).OnClick));
                    }
                    else if (control is GUIListBox)
                    {
                        objectEvents.Add(new GUIEventReference(obj, "OnSelectionChanged", (control as GUIListBox).OnSelectionChanged));
                    }
                    else if (control is GUISlider)
                    {
                        objectEvents.Add(new GUIEventReference(obj, "OnChange", (control as GUISlider).OnChange));
                    }
                    else if (control is GUITextBox)
                    {
                        objectEvents.Add(new GUIEventReference(obj, "OnActivate", (control as GUITextBox).OnActivate));
                    }
                }

                var functionNames = objectEvents.Select(evt => evt.FunctionName);
                var missing = _agsEditor.Tasks.TestMissingEventHandlers(ngui.ScriptModule, functionNames.ToArray());
                if (missing == null || missing.Count == 0)
                    continue;

                foreach (var miss in missing)
                {
                    GUIEventReference evtRef = objectEvents[miss];
                    GUIObjectWithEvents guiObject = evtRef.GUIObject;
                    if (guiObject.GUI != null)
                    {
                        errors.Add(new CompileWarning($"GUI ({ngui.ID}) {ngui.Name}'s event {evtRef.EventName} function \"{evtRef.FunctionName}\" not found in script {ngui.ScriptModule}."));
                    }
                    else if (guiObject.Control != null)
                    {
                        string typeName = guiObject.Control.ControlType;
                        int objid = guiObject.Control.ID;
                        string scriptName = guiObject.Control.Name;

                        errors.Add(new CompileWarning($"GUI ({ngui.ID}) {ngui.Name}: {typeName} #{objid} {scriptName}'s event {evtRef.EventName} function \"{evtRef.FunctionName}\" not found in script {ngui.ScriptModule}."));
                    }
                }
            }
        }
    }
}
