using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Xml;
using AGS.Types;
using AGS.Editor.Resources;

namespace AGS.Editor.Components
{
    public abstract class BaseComponentWithFolders<ItemType,FolderType> : BaseComponent 
        where ItemType : IToXml
        where FolderType : BaseFolderCollection<ItemType,FolderType>
    {
        protected abstract FolderType GetRootFolder();
        protected abstract ProjectTreeItem CreateTreeItemForItem(ItemType item);
        protected abstract void AddExtraCommandsToFolderContextMenu(string controlID, IList<MenuCommand> menu);
        protected abstract void ItemCommandClick(string controlID);
        protected abstract FolderType CreateFolderObject(string name, FolderType parentFolder);
        protected abstract string GetFolderDeleteConfirmationText();
        protected abstract bool CanFolderBeDeleted(FolderType folder);
        protected abstract void DeleteResourcesUsedByFolder(FolderType folder);
        protected virtual void AddExtraManualNodesToTree() { }

        protected readonly string TOP_LEVEL_COMMAND_ID;
        protected readonly string NODE_ID_PREFIX_FOLDER;
        private readonly string COMMAND_NEW_FOLDER;
        private readonly string COMMAND_NEW_SUB_FOLDER;
        private readonly string COMMAND_RENAME_FOLDER;
        private readonly string COMMAND_DELETE_FOLDER;
        protected readonly string ITEM_COMMAND_PREFIX;

        protected Dictionary<string, FolderType> _folders = new Dictionary<string, FolderType>();
        protected Dictionary<string, ItemType> _items = new Dictionary<string, ItemType>();

        protected string _rightClickedID;

        public BaseComponentWithFolders(GUIController guiController, AGSEditor agsEditor, string topLevelCommandId)
            : base(guiController, agsEditor)
        {
            string typeName = typeof(ItemType).Name;
            TOP_LEVEL_COMMAND_ID = topLevelCommandId;
            NODE_ID_PREFIX_FOLDER = string.Format("{0}Fldr", typeName);
            COMMAND_NEW_FOLDER = string.Format("New{0}Folder", typeName);
            COMMAND_NEW_SUB_FOLDER = string.Format("New{0}SubFolder", typeName);
            COMMAND_RENAME_FOLDER = string.Format("Rename{0}Folder", typeName);
            COMMAND_DELETE_FOLDER = string.Format("Delete{0}Folder", typeName);
            ITEM_COMMAND_PREFIX = typeName;

            _guiController.RegisterIcon("GenericFolderIcon", ResourceManager.GetIcon("folder.ico"));
        }

        public sealed override void CommandClick(string controlID)
        {
            if (controlID == COMMAND_NEW_FOLDER)
            {
                CreateSubFolder(TOP_LEVEL_COMMAND_ID, this.GetRootFolder());
            }
            else if (controlID == COMMAND_NEW_SUB_FOLDER)
            {
                CreateSubFolder(_rightClickedID, _folders[_rightClickedID]);
            }
            else if (controlID == COMMAND_RENAME_FOLDER)
            {
                _guiController.ProjectTree.BeginLabelEdit(this, _rightClickedID);
            }
            else if (controlID == COMMAND_DELETE_FOLDER)
            {
                if (_guiController.ShowQuestion(GetFolderDeleteConfirmationText()) == System.Windows.Forms.DialogResult.Yes)
                {
                    DeleteFolder(_folders[_rightClickedID]);
                }
            }
            else
            {
                ItemCommandClick(controlID);
            }
        }

        private void DeleteFolder(FolderType deleteThis)
        {
            if (CanFolderBeDeleted(deleteThis))
            {
                DeleteResourcesUsedByFolder(deleteThis);

                FolderType parent = FindParentFolder(this.GetRootFolder(), deleteThis);
                if (parent == null)
                {
                    throw new AGSEditorException("Folder to be deleted has no parent");
                }

                parent.SubFolders.Remove(deleteThis);
                RePopulateTreeView(GetNodeIDForFolder(parent));
                _guiController.ProjectTree.ExpandNode(this, GetNodeIDForFolder(parent));
            }
        }

        protected void CreateSubFolder(string parentNodeID, FolderType parentFolder)
        {
            _guiController.ProjectTree.StartFromNode(this, parentNodeID);
            FolderType newFolder = CreateFolderObject("New folder", parentFolder);
            parentFolder.SubFolders.Add(newFolder);
            string newNodeID = AddNodeForFolder(newFolder);
            _guiController.ProjectTree.BeginLabelEdit(this, newNodeID);
        }

        public override IList<MenuCommand> GetContextMenu(string controlID)
        {
            _rightClickedID = controlID;
            IList<MenuCommand> menu = new List<MenuCommand>();
            if (controlID == TOP_LEVEL_COMMAND_ID)
            {
                menu.Add(new MenuCommand(COMMAND_NEW_FOLDER, "New Folder", null));
                AddExtraCommandsToFolderContextMenu(controlID, menu);
            }
            else if (controlID.StartsWith(NODE_ID_PREFIX_FOLDER))
            {
                menu.Add(new MenuCommand(COMMAND_RENAME_FOLDER, "Rename", null));
                menu.Add(new MenuCommand(COMMAND_DELETE_FOLDER, "Delete", null));
                menu.Add(MenuCommand.Separator);
                menu.Add(new MenuCommand(COMMAND_NEW_SUB_FOLDER, "New Sub-Folder", null));
                AddExtraCommandsToFolderContextMenu(controlID, menu);
            }
            return menu;
        }

        private string AddNodeForFolder(FolderType folder)
        {
            string nodeID = NODE_ID_PREFIX_FOLDER + folder.Name;
            while (_folders.ContainsKey(nodeID))
            {
                nodeID = nodeID + "A";
            }
            _folders.Add(nodeID, folder);

            ProjectTreeItem newItem = _guiController.ProjectTree.AddTreeBranch(this, nodeID, folder.Name, "GenericFolderIcon");
            newItem.AllowLabelEdit = true;
            newItem.LabelTextProperty = folder.GetType().GetProperty("Name");
            newItem.LabelTextDataSource = folder;
            newItem.AllowDragging = true;
            newItem.CanDropHere = new ProjectTreeItem.CanDropHereDelegate(ProjectTreeItem_CanDropHere);
            newItem.DropHere = new ProjectTreeItem.DropHereDelegate(ProjectTreeItem_DropHere);

            return nodeID;
        }

        private void PopulateTreeForFolder(FolderType folder, string folderID)
        {
            foreach (FolderType subFolder in folder.SubFolders)
            {
                _guiController.ProjectTree.StartFromNode(this, folderID);
                string newFolderID = AddNodeForFolder(subFolder);
                PopulateTreeForFolder(subFolder, newFolderID);
            }

            _guiController.ProjectTree.StartFromNode(this, folderID);
            foreach (ItemType item in folder.Items)
            {
                AddTreeNodeForItem(item);
            }
        }

        /// <summary>
        /// Adds a tree leaf for the item, and returns the Command ID
        /// </summary>
        protected string AddTreeNodeForItem(ItemType item)
        {
            ProjectTreeItem treeItem = CreateTreeItemForItem(item);
            treeItem.AllowDragging = true;
            treeItem.CanDropHere = new ProjectTreeItem.CanDropHereDelegate(ProjectTreeItem_CanDropHere);
            treeItem.DropHere = new ProjectTreeItem.DropHereDelegate(ProjectTreeItem_DropHere);
            _items.Add(treeItem.ID, item);

            return treeItem.ID;
        }

        protected bool IsFolderNode(string commandId)
        {
            return ((commandId.StartsWith(NODE_ID_PREFIX_FOLDER)) ||
                    (commandId == TOP_LEVEL_COMMAND_ID));
        }

        private bool ProjectTreeItem_CanDropHere(ProjectTreeItem source, ProjectTreeItem target)
        {
            return IsFolderNode(target.ID);
        }

        private bool IsValidFolderMove(FolderType source, FolderType target)
        {
            foreach (FolderType subFolder in source.SubFolders)
            {
                if (!IsValidFolderMove(subFolder, target))
                {
                    return false;
                }
            }

            return (source != target);
        }

        private void ProjectTreeItem_DropHere(ProjectTreeItem source, ProjectTreeItem target)
        {
            FolderType targetFolder;

            if (target.ID == TOP_LEVEL_COMMAND_ID)
            {
                targetFolder = this.GetRootFolder();
            }
            else
            {
                targetFolder = _folders[target.ID];
            }

            if (_folders.ContainsKey(source.ID))
            {
                FolderType folderToMove = _folders[source.ID];
                if (!IsValidFolderMove(folderToMove, targetFolder))
                {
                    _guiController.ShowMessage("You cannot move a folder into itself or one of its children.", MessageBoxIconType.Warning);
                    return;
                }

                FolderType parentOfSource = FindParentFolder(this.GetRootFolder(), folderToMove);
                if (parentOfSource == null)
                {
                    throw new AGSEditorException("Folder being moved has no parent?");
                }

                if (parentOfSource == targetFolder)
                {
                    return;
                }

                parentOfSource.SubFolders.Remove(folderToMove);
                targetFolder.SubFolders.Add(folderToMove);
            }
            else
            {
                ItemType viewToMove = _items[source.ID];
                FolderType sourceFolder = FindFolderThatContainsItem(this.GetRootFolder(), viewToMove);
                if (sourceFolder == null)
                {
                    throw new AGSEditorException("Source view was not in a folder");
                }

                sourceFolder.Items.Remove(viewToMove);
                targetFolder.Items.Add(viewToMove);
            }

            RePopulateTreeView(target.ID);
            _guiController.ProjectTree.SelectNode(this, source.ID);
        }

        protected FolderType FindFolderThatContainsItem(FolderType rootFolder, ItemType view)
        {
            foreach (FolderType subFolder in rootFolder.SubFolders)
            {
                FolderType result = FindFolderThatContainsItem(subFolder, view);
                if (result != null)
                {
                    return result;
                }
            }

            if (rootFolder.Items.Contains(view))
            {
                return rootFolder;
            }

            return null;
        }

        private FolderType FindParentFolder(FolderType rootFolder, FolderType folderToFindParentOf)
        {
            foreach (FolderType subFolder in rootFolder.SubFolders)
            {
                if (subFolder == folderToFindParentOf)
                {
                    return rootFolder;
                }

                FolderType result = FindParentFolder(subFolder, folderToFindParentOf);
                if (result != null)
                {
                    return result;
                }
            }

            return null;
        }

        protected string GetNodeIDForFolder(FolderType folder)
        {
            foreach (KeyValuePair<string, FolderType> entry in _folders)
            {
                if (entry.Value == folder)
                {
                    return entry.Key;
                }
            }
            return null;
        }

        protected void RePopulateTreeView(string selectedNodeID)
        {
            RePopulateTreeView();
            _guiController.ProjectTree.SelectNode(this, selectedNodeID);
        }

        protected void RePopulateTreeView()
        {
            _items.Clear();
            _folders.Clear();
            _folders.Add(TOP_LEVEL_COMMAND_ID, this.GetRootFolder());
            _guiController.ProjectTree.RemoveAllChildNodes(this, TOP_LEVEL_COMMAND_ID);
            _guiController.ProjectTree.StartFromNode(this, TOP_LEVEL_COMMAND_ID);
            AddExtraManualNodesToTree();
            PopulateTreeForFolder(this.GetRootFolder(), TOP_LEVEL_COMMAND_ID);
        }

    }
}
