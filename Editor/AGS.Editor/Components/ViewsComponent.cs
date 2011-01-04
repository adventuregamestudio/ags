using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Xml;
using AGS.Types;
using AGS.Editor.Resources;

namespace AGS.Editor.Components
{
    class ViewsComponent : BaseComponentWithFolders<View, ViewFolder>
    {
        private const string COMMAND_NEW_VIEW = "NewView";
        private const string COMMAND_RENAME = "RenameView";
		private const string COMMAND_DELETE = "DeleteView";

        private Dictionary<View, ContentDocument> _documents;
        private Game.ViewListUpdatedHandler _viewsUpdatedHandler;
        private Game _eventHookedToGame = null;

        public ViewsComponent(GUIController guiController, AGSEditor agsEditor)
            : base(guiController, agsEditor, "ViewEditor")
        {
            _documents = new Dictionary<View, ContentDocument>();
            _viewsUpdatedHandler = new Game.ViewListUpdatedHandler(CurrentGame_ViewListUpdated);
            _guiController.RegisterIcon("ViewsIcon", ResourceManager.GetIcon("view.ico"));
            _guiController.RegisterIcon("ViewIcon", ResourceManager.GetIcon("view-item.ico"));
            _guiController.ProjectTree.AddTreeRoot(this, TOP_LEVEL_COMMAND_ID, "Views", "ViewsIcon");
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
                        ViewFolder parentFolder = this.FindFolderThatContainsItem(_agsEditor.CurrentGame.RootViewFolder, viewToDelete);
						parentFolder.Views.Remove(viewToDelete);
						MarkViewAsDeleted(viewToDelete);

						RePopulateTreeView(GetNodeIDForFolder(parentFolder));
                        _guiController.ProjectTree.ExpandNode(this, GetNodeIDForFolder(parentFolder));
                    }
				}
			}
			else if (controlID == COMMAND_NEW_VIEW)
			{
				View newView = new View();
				newView.ID = _agsEditor.CurrentGame.FindAndAllocateAvailableViewID();
				newView.Name = "View" + newView.ID;
				_folders[_rightClickedID].Views.Add(newView);

				_guiController.ProjectTree.StartFromNode(this, _rightClickedID);
				string newNodeID = AddTreeNodeForItem(newView);
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

		private void MarkAllViewsInFolderAsDeleted(ViewFolder folder)
		{
			foreach (View view in folder.Views)
			{
				MarkViewAsDeleted(view);
			}

			foreach (ViewFolder subFolder in folder.SubFolders)
			{
				MarkAllViewsInFolderAsDeleted(subFolder);
			}
		}

		private void ShowOrAddPane(View chosenItem)
		{
			if (!_documents.ContainsKey(chosenItem))
			{
				_documents.Add(chosenItem, new ContentDocument(new ViewEditor(chosenItem), chosenItem.WindowTitle, this, null));
				_documents[chosenItem].SelectedPropertyGridObject = chosenItem;
			}
			_guiController.AddOrShowPane(_documents[chosenItem]);
			_guiController.ShowCuppit("Views are how you set up animations in AGS. Each View contains a set of related animations, each of which is a Loop consisting of several Frames.\nFor character walking animations, a view consists of one loop for each direction.", "Views introduction");
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
                    RePopulateTreeView(GetNodeIDForView(itemBeingEdited));
                    UpdateOpenWindowTitles();
                }
            }
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            IList<MenuCommand> menu = base.GetContextMenu(controlID);
            
            if ((controlID.StartsWith(ITEM_COMMAND_PREFIX)) &&
                (!IsFolderNode(controlID)))
            {
                menu.Add(new MenuCommand(COMMAND_RENAME, "Rename", null));
				menu.Add(new MenuCommand(COMMAND_DELETE, "Delete", null));
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

			foreach (Character character in game.Characters)
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

        protected override void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu)
        {
            menu.Add(new MenuCommand(COMMAND_NEW_VIEW, "New View", null));
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

        protected override void DeleteResourcesUsedByFolder(ViewFolder folder)
        {
            MarkAllViewsInFolderAsDeleted(folder);
        }

        protected override ViewFolder CreateFolderObject(string name, ViewFolder parentFolder)
        {
            return new ViewFolder(name);
        }

        protected override string GetFolderDeleteConfirmationText()
        {
            return "Are you sure you want to delete this folder and all its views?" + Environment.NewLine + Environment.NewLine + "If any of the views are used as animations anywhere it could cause crashes in the game.";
        }

    }
}
