using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Xml;
using AGS.Types;

namespace AGS.Editor.Components
{
    class CursorsComponent : BaseComponent
    {
        private const string TOP_LEVEL_COMMAND_ID = "Cursors";
        private const string COMMAND_NEW_ITEM = "NewCursor";
        private const string COMMAND_DELETE_ITEM = "DeleteCursor";
        private const int BUILT_IN_CURSORS = 8;
        private const string ICON_KEY = "CursorsIcon";

        private Dictionary<MouseCursor, ContentDocument> _documents;
        private MouseCursor _itemRightClicked = null;

        public CursorsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor)
        {
            _documents = new Dictionary<MouseCursor, ContentDocument>();
            _guiController.RegisterIcon(ICON_KEY, Resources.ResourceManager.GetIcon("cursor.ico"));
            _guiController.RegisterIcon("CursorIcon", Resources.ResourceManager.GetIcon("cursor-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Mouse cursors", ICON_KEY);
            RePopulateTreeView();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.Cursors; }
        }

        public override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_ITEM)
            {
                IList<MouseCursor> items = _agsEditor.CurrentGame.Cursors;
                MouseCursor newItem = new MouseCursor();
                newItem.ID = items.Count;
                newItem.Name = "Cursor" + newItem.ID;
                items.Add(newItem);
                SyncCursorsWithInteractionSchema();
                _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
                _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(newItem), GetNodeLabel(newItem), "CursorIcon");
                _guiController.ProjectTree.SelectNode(this, GetNodeID(newItem));
				ShowOrAddPane(newItem);
            }
            else if (controlID == COMMAND_DELETE_ITEM)
            {
                if (MessageBox.Show("Are you sure you want to delete this cursor? Doing so will break any scripts that refer to cursors by their number.", "Confirm delete", MessageBoxButtons.YesNo, MessageBoxIcon.Question) == DialogResult.Yes)
                {
                    int removingID = _itemRightClicked.ID;
                    foreach (MouseCursor item in _agsEditor.CurrentGame.Cursors)
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
                    _agsEditor.CurrentGame.Cursors.Remove(_itemRightClicked);
                    SyncCursorsWithInteractionSchema();
                    RePopulateTreeView();
                }
            }
            else if (controlID != TOP_LEVEL_COMMAND_ID)
            {
                MouseCursor chosenCursor = _agsEditor.CurrentGame.Cursors[Convert.ToInt32(controlID.Substring(3))];
				ShowOrAddPane(chosenCursor);
			}
        }

		private void ShowOrAddPane(MouseCursor chosenCursor)
		{
            ContentDocument document;
			if (!_documents.TryGetValue(chosenCursor, out document)
                || document.Control.IsDisposed)
			{
                document = new ContentDocument(new CursorEditor(chosenCursor),
                    chosenCursor.WindowTitle, this, ICON_KEY, 
                    ConstructPropertyObjectList(chosenCursor));
                
                _documents[chosenCursor] = document;
                document.SelectedPropertyGridObject = chosenCursor;
			}
            document.TreeNodeID = GetNodeID(chosenCursor);
            _guiController.AddOrShowPane(document);
		}

        public override IList<string> GetManagedScriptElements()
        {
            return new string[] { "CursorMode" };
        }

        public override bool ShowItemPaneByName(string name)
        {
            foreach (MouseCursor mc in _agsEditor.CurrentGame.Cursors)
            {
                if (mc.ScriptID == name)
                {
                    MouseCursor chosenCursor = mc;
                    _guiController.ProjectTree.SelectNode(this, GetNodeID(chosenCursor));
                    ShowOrAddPane(chosenCursor);
                    return true;
                }
            }
            return false;
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == "Name")
            {
                RePopulateTreeView();

                foreach (ContentDocument doc in _documents.Values)
                {
                    doc.Name = ((CursorEditor)doc.Control).ItemToEdit.WindowTitle;
                }
            }

            // TODO: if we had a reference to the current object (why we dont have one in PropertyChanged??),
            // then we could also check the current state of CreateEvent property and optimize following a bit.
            if (propertyName == "CreateEvent" ||
                (propertyName == "Name" || propertyName == "EventLabel" || propertyName == "EventFunctionName"))
            {
                SyncCursorsWithInteractionSchema();
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_ITEM, "New Cursor", null));
            }
            else
            {
                int cursorID = Convert.ToInt32(controlID.Substring(3));
                _itemRightClicked = _agsEditor.CurrentGame.Cursors[cursorID];
                menu.Add(new MenuCommand(COMMAND_DELETE_ITEM, "Delete this cursor", null));
                if (cursorID < BUILT_IN_CURSORS)
                {
                    // can't delete built-in cursors
                    menu[menu.Count - 1].Enabled = false;
                }
            }
            return menu;
        }

        public override void BeforeSaveGame()
        {
            foreach (ContentDocument doc in _documents.Values)
            {
                ((CursorEditor)doc.Control).SaveData();
            }
        }

        public override void RefreshDataFromGame()
        {
            SyncCursorsWithInteractionSchema();

            foreach (ContentDocument doc in _documents.Values)
            {
                _guiController.RemovePaneIfExists(doc);
                doc.Dispose();
            }
            _documents.Clear();

            RePopulateTreeView();
        }

        private string GetNodeID(MouseCursor item)
        {
            return "Cur" + item.ID;
        }

        private string GetNodeLabel(MouseCursor item)
        {
            return item.ID.ToString() + ": " + item.Name;
        }

        private void RePopulateTreeView()
        {
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            foreach (MouseCursor item in _agsEditor.CurrentGame.Cursors)
            {
                _guiController.ProjectTree.AddTreeLeaf(this, GetNodeID(item), GetNodeLabel(item), "CursorIcon");
            }

            if (_documents.ContainsValue(_guiController.ActivePane))
            {
                CursorEditor editor = (CursorEditor)_guiController.ActivePane.Control;
                _guiController.ProjectTree.SelectNode(this, GetNodeID(editor.ItemToEdit));
            }
            else if (_agsEditor.CurrentGame.Cursors.Count > 0)
            {
                _guiController.ProjectTree.SelectNode(this, "Cur0");
            }
        }

        private Dictionary<string, object> ConstructPropertyObjectList(MouseCursor cursor)
        {
            Dictionary<string, object> list = new Dictionary<string, object>();
            list.Add(cursor.PropertyGridTitle, cursor);
            return list;
        }

        private void SyncCursorsWithInteractionSchema()
        {
            // TODO: maybe don't reset everything every time, but only do necessary change to the list
            var cursors = _agsEditor.CurrentGame.Cursors;
            var schema = InteractionSchema.Instance;
            List<InteractionEvent> events = new List<InteractionEvent>();
            foreach (var cursor in cursors)
            {
                if (cursor.CreateEvent)
                {
                    string eventName = Types.Utilities.RemoveInvalidCharactersFromScriptName(cursor.Name);
                    string displayName = string.IsNullOrWhiteSpace(cursor.EventLabel) ? cursor.Name : cursor.EventLabel;
                    string functionSuffix = string.IsNullOrWhiteSpace(cursor.EventFunctionName) ? eventName : cursor.EventFunctionName;
                    events.Add(new InteractionEvent(cursor.ID, eventName, displayName, functionSuffix));
                }
            }
            schema.Events = events.ToArray();
        }
    }
}
