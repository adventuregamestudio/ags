using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Xml;
using AGS.Types;
using AGS.Editor.Resources;
using AGS.Editor.TextProcessing;

namespace AGS.Editor.Components
{
    class ViewsComponent : BaseComponentWithFolders<View, ViewFolder>
    {
        private const string COMMAND_NEW_VIEW = "NewView";
        private const string COMMAND_RENAME = "RenameView";
		private const string COMMAND_DELETE = "DeleteView";
        private const string COMMAND_CHANGE_ID = "ChangeViewID";
        private const string COMMAND_FIND_ALL_USAGES = "FindAllUsages";
        private const string ICON_KEY = "ViewsIcon";
        
        private Dictionary<View, ContentDocument> _documents;
        private Game.ViewListUpdatedHandler _viewsUpdatedHandler;
        private Game _eventHookedToGame = null;

        public ViewsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, "ViewEditor")
        {
            _documents = new Dictionary<View, ContentDocument>();
            _viewsUpdatedHandler = new Game.ViewListUpdatedHandler(CurrentGame_ViewListUpdated);
            _guiController.RegisterIcon(ICON_KEY, ResourceManager.GetIcon("view.ico"));
            _guiController.RegisterIcon("ViewIcon", ResourceManager.GetIcon("view-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Views", ICON_KEY);
			_guiController.ProjectTree.OnAfterLabelEdit += new ProjectTree.AfterLabelEditHandler(ProjectTree_OnAfterLabelEdit);
			RefreshDataFromGame();
        }

        public override string ComponentID
        {
            get { return ComponentIDs.ViewEditor; }
        }

        protected override void ItemCommandClick(string controlID)
        {
			if (controlID == COMMAND_DELETE)
			{
				View viewToDelete = _items[_rightClickedID];
				if (_guiController.ShowQuestion("Are you sure you want to delete view '" + viewToDelete.Name + "'?" + Environment.NewLine + Environment.NewLine + "If it is used as an animation anywhere it could cause crashes in the game.") == System.Windows.Forms.DialogResult.Yes)
				{
					string usage = GetViewUsageReport(viewToDelete.ID);
					if (usage != null)
					{
						_guiController.ShowMessage(usage, MessageBoxIconType.Warning);
					}
					else
					{
                        DeleteSingleItem(viewToDelete);                        
                    }
				}
			}
            else if (controlID == COMMAND_CHANGE_ID)
            {
                View viewClicked = _items[_rightClickedID];
                int oldNumber = viewClicked.ID;
                int newNumber = Factory.GUIController.ShowChangeObjectIDDialog("View", oldNumber, 1, _items.Count);
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
                _agsEditor.CurrentGame.GetAndAllocateViewID(newNumber);
                viewClicked.ID = newNumber;
                OnItemIDChanged(viewClicked);
            }
            else if (controlID == COMMAND_FIND_ALL_USAGES)
            {
                FindAllUsages findAllUsages = new FindAllUsages(null, null, null, _agsEditor);
                View viewToFind = _items[_rightClickedID];
                findAllUsages.Find(null, viewToFind.Name);
            }
            else if (controlID == COMMAND_NEW_VIEW)
            {
                View newView = new View();
                newView.ID = _agsEditor.CurrentGame.FindAndAllocateAvailableViewID();
                newView.Name = "View" + newView.ID;

                string newNodeID = AddSingleItem(newView);
                ShowOrAddPane(newView);
                _guiController.ProjectTree.BeginLabelEdit(this, newNodeID);
            }
            else if (controlID == COMMAND_RENAME)
            {
                _guiController.ProjectTree.BeginLabelEdit(this, _rightClickedID);
            }
            else if ((!controlID.StartsWith(NODE_ID_PREFIX_FOLDER)) &&
                     (controlID != TOP_LEVEL_COMMAND_ID))
            {
                View chosenItem = _items[controlID];
                ShowOrAddPane(chosenItem);
            }
        }

		private void MarkViewAsDeleted(View viewToDelete)
		{
			_agsEditor.CurrentGame.ViewDeleted(viewToDelete.ID);

			if (_documents.ContainsKey(viewToDelete))
			{
				_guiController.RemovePaneIfExists(_documents[viewToDelete]);
				_documents.Remove(viewToDelete);
			}
		}

		private string CheckIfAnyViewsInFolderTreeAreUsed(ViewFolder startFromFolder)
		{
			foreach (View view in startFromFolder.Views)
			{
				string result = GetViewUsageReport(view.ID);
				if (result != null)
				{
					return result;
				}
			}

			foreach (ViewFolder subFolder in startFromFolder.SubFolders)
			{
				string result = CheckIfAnyViewsInFolderTreeAreUsed(subFolder);
				if (result != null)
				{
					return result;
				}
			}

			return null;
		}

		private void ShowOrAddPane(View chosenItem)
		{
            ContentDocument document;
			if (!_documents.TryGetValue(chosenItem, out document)
                || document.Control.IsDisposed)
			{
                document = new ContentDocument(new ViewEditor(chosenItem), chosenItem.WindowTitle, this,
                    ICON_KEY, null);
                _documents[chosenItem] = document;
                document.SelectedPropertyGridObject = chosenItem;
			}
            document.TreeNodeID = GetNodeIDForView(chosenItem);
            _guiController.AddOrShowPane(document);
		}

		private void UpdateOpenWindowTitles()
		{
			foreach (ContentDocument doc in _documents.Values)
			{
				doc.Name = ((ViewEditor)doc.Control).ViewToEdit.WindowTitle;
			}
		}

        private void ProjectTree_OnAfterLabelEdit(string commandID, ProjectTreeItem treeItem)
        {
            if (commandID.StartsWith(ITEM_COMMAND_PREFIX))
            {
                if (!commandID.StartsWith(NODE_ID_PREFIX_FOLDER))
                {
                    View itemBeingEdited = (View)treeItem.LabelTextDataSource;

                    if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.Name.ToUpper(), itemBeingEdited))
                    {
                        _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIconType.Warning);
                        itemBeingEdited.Name = treeItem.LabelTextBeforeLabelEdit;
                        treeItem.TreeNode.Text = itemBeingEdited.NameAndID;
                        return;
                    }
                }
                UpdateOpenWindowTitles();
                _guiController.DocumentTitlesChanged();
            }
        }

        private void OnItemIDChanged(View item)
        {
            RePopulateTreeView(GetNodeIDForView(item));
            UpdateOpenWindowTitles();
        }

        public override void PropertyChanged(string propertyName, object oldValue)
        {
            if (propertyName == "Name")
            {
                View itemBeingEdited = ((ViewEditor)_guiController.ActivePane.Control).ViewToEdit;
                if (_agsEditor.CurrentGame.IsScriptNameAlreadyUsed(itemBeingEdited.Name.ToUpper(), itemBeingEdited))
                {
                    _guiController.ShowMessage("This script name is already used by another item.", MessageBoxIconType.Warning);
                    itemBeingEdited.Name = (string)oldValue;
                }
                else
                {
                    OnItemIDChanged(itemBeingEdited);
                }
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            
            if ((controlID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!IsFolderNode(controlID)))
            {
                menu.Add(new MenuCommand(COMMAND_CHANGE_ID, "Change View ID", null));
                menu.Add(new MenuCommand(COMMAND_RENAME, "Rename", null));
				menu.Add(new MenuCommand(COMMAND_DELETE, "Delete", null));
                View view = _items[_rightClickedID];
                menu.Add(new MenuCommand(COMMAND_FIND_ALL_USAGES, "Find all usages of " + view.Name, null));
            }
            return menu;
        }

        public override void RefreshDataFromGame()
        {
            if (_eventHookedToGame != null)
            {
                _eventHookedToGame.ViewListUpdated -= _viewsUpdatedHandler;
            }

            foreach (ContentDocument doc in _documents.Values)
            {
                _guiController.RemovePaneIfExists(doc);
                doc.Dispose();
            }
            _documents.Clear();

            RePopulateTreeView();

            _eventHookedToGame = _agsEditor.CurrentGame;
            _eventHookedToGame.ViewListUpdated += _viewsUpdatedHandler;
        }

        private void CurrentGame_ViewListUpdated()
        {
            RePopulateTreeView();
        }

		private string GetViewUsageReport(int viewNumber)
		{
			StringBuilder usageReport = new StringBuilder(5000);
			Game game = Factory.AGSEditor.CurrentGame;

            foreach (Character character in game.RootCharacterFolder.AllItemsFlat)
			{
				string charText = "character " + character.ID + " (" + character.RealName + ")";

				if (character.BlinkingView == viewNumber)
				{
					usageReport.AppendLine("Blinking view for " + charText);
				}
				if (character.NormalView == viewNumber)
				{
					usageReport.AppendLine("Normal view for " + charText);
				}
				if (character.IdleView == viewNumber)
				{
					usageReport.AppendLine("Idle view for " + charText);
				}
				if (character.ThinkingView == viewNumber)
				{
					usageReport.AppendLine("Thinking view for " + charText);
				}
				if (character.SpeechView == viewNumber)
				{
					usageReport.AppendLine("Speech view for " + charText);
				}
			}

			foreach (MouseCursor item in game.Cursors)
			{
				if (item.View == viewNumber)
				{
					usageReport.AppendLine("Mouse cursor " + item.ID + " (" + item.Name + ")");
				}
			}

			if (usageReport.Length > 0)
			{
				string resultText = "View " + viewNumber + " is used in the following places. It may also be used in animations controlled by script commands; we cannot detect those uses automatically.";
				resultText += Environment.NewLine + Environment.NewLine + usageReport.ToString();
				return resultText;
			}
			return null;
		}

        private string GetNodeIDForView(View item)
        {
            return ITEM_COMMAND_PREFIX + item.ID;
        }

        protected override ViewFolder GetRootFolder()
        {
            return _agsEditor.CurrentGame.RootViewFolder;
        }

        protected override ProjectTreeItem CreateTreeItemForItem(View item)
        {
            string nodeID = GetNodeIDForView(item);
            ProjectTreeItem treeItem = (ProjectTreeItem)_guiController.ProjectTree.AddTreeLeaf(this, nodeID, item.ID.ToString() + ": " + item.Name, "ViewIcon");
            treeItem.AllowLabelEdit = true;
            treeItem.LabelTextProperty = item.GetType().GetProperty("Name");
            treeItem.LabelTextDescriptionProperty = item.GetType().GetProperty("NameAndID");
            treeItem.LabelTextDataSource = item;
            return treeItem;
        }

        protected override void AddNewItemCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_VIEW, "New View", null));
        }

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            // No more commands in this menu
        }

        protected override bool CanFolderBeDeleted(ViewFolder folder)
        {
            string usageReport = CheckIfAnyViewsInFolderTreeAreUsed(folder);
            if (usageReport != null)
            {
                _guiController.ShowMessage(usageReport, MessageBoxIconType.Warning);
                return false;
            }
            return true;
        }

        protected override void DeleteResourcesUsedByItem(View item)
        {
            MarkViewAsDeleted(item);
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its views?" + Environment.NewLine + Environment.NewLine + "If any of the views are used as animations anywhere it could cause crashes in the game.";
        }

    }
}
