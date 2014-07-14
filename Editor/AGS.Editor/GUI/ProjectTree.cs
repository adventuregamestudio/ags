using AGS.Types;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class ProjectTree : IProjectTree
    {
        public const string DEFAULT_ICON_KEY = "DefaultTreeIcon";

        public delegate void MenuClickHandler(string menuItemID);
        public event MenuClickHandler OnContextMenuClick;
        public delegate void AfterLabelEditHandler(string commandID, ProjectTreeItem treeItem);
        public event AfterLabelEditHandler OnAfterLabelEdit;
        public event BeforeShowContextMenuHandler BeforeShowContextMenu;

        private Dictionary<string, IEditorComponent> _treeNodes;
        private TreeView _projectTree;
        private TreeNode _lastAddedNode = null;
        private DateTime _expandedAtTime = DateTime.MinValue;
        private string _selectedNode;
        private Color? _treeNodesBackgroundColor;
        private TreeNode _dropHoveredNode;
        private DateTime _timeOfDragDropHoverStart;


        public ProjectTree(TreeView projectTree)
        {
            Factory.GUIController.RegisterIcon(DEFAULT_ICON_KEY, Resources.ResourceManager.GetIcon("iconplug.ico"));
            projectTree.ImageKey = DEFAULT_ICON_KEY;
            projectTree.SelectedImageKey = DEFAULT_ICON_KEY;

            _projectTree = projectTree;
            _treeNodes = new Dictionary<string, IEditorComponent>();

            _projectTree.MouseClick += new System.Windows.Forms.MouseEventHandler(this.projectTree_MouseClick);
            _projectTree.NodeMouseDoubleClick += new System.Windows.Forms.TreeNodeMouseClickEventHandler(this.projectTree_NodeMouseDoubleClick);
            _projectTree.DoubleClick += new System.EventHandler(this.projectTree_DoubleClick);
            _projectTree.AfterLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.projectTree_AfterLabelEdit);
            _projectTree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.projectTree_AfterSelect);
            _projectTree.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.projectTree_KeyPress);
            _projectTree.BeforeLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.projectTree_BeforeLabelEdit);
            _projectTree.BeforeExpand += new TreeViewCancelEventHandler(_projectTree_BeforeExpand);
            _projectTree.BeforeCollapse += new TreeViewCancelEventHandler(_projectTree_BeforeCollapse);
            _projectTree.ItemDrag += new ItemDragEventHandler(projectTree_ItemDrag);
            _projectTree.DragOver += new DragEventHandler(projectTree_DragOver);
            _projectTree.DragDrop += new DragEventHandler(projectTree_DragDrop);
        }

        private void _projectTree_BeforeCollapse(object sender, TreeViewCancelEventArgs e)
        {
            _expandedAtTime = DateTime.Now;
        }

        private void _projectTree_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            _expandedAtTime = DateTime.Now;
        }

        public void CollapseAll()
        {
            _projectTree.CollapseAll();
        }

        public IProjectTreeItem AddTreeLeaf(IEditorComponent plugin, string id, string name, string iconKey)
        {
            return AddTreeLeaf(plugin, id, name, iconKey, false);
        }

        private void RegisterTreeNode(string id, IEditorComponent plugin)
        {
            if (_treeNodes.ContainsKey(id))
            {
                if (_treeNodes[id] != plugin)
                {
                    throw new AGSEditorException("Tree node " + id + " is already registered to another component!");
                }
            }
            else
            {
                _treeNodes.Add(id, plugin);
            }
        }

        public ProjectTreeItem AddTreeBranch(IEditorComponent component, string id, string name, string iconKey)
        {
            RegisterTreeNode(id, component);

            iconKey = SetDefaultIconIfNoneProvided(iconKey);
            if (_lastAddedNode != null)
            {
                _lastAddedNode = _lastAddedNode.Nodes.Add(id, name, iconKey, iconKey);
            }
            else
            {
                _lastAddedNode = this._projectTree.Nodes.Add(id, name, iconKey, iconKey);
            }
            ProjectTreeItem newItem = new ProjectTreeItem(id, _lastAddedNode);
            _lastAddedNode.Tag = newItem;
            return newItem;
        }

        public IProjectTreeItem AddTreeLeaf(IEditorComponent component, string id, string name, string iconKey, bool greyedOut)
        {
            RegisterTreeNode(id, component);

            TreeNode newNode;
            iconKey = SetDefaultIconIfNoneProvided(iconKey);
            if (_lastAddedNode != null)
            {
                newNode = _lastAddedNode.Nodes.Add(id, name, iconKey, iconKey);
            }
            else
            {
                newNode = this._projectTree.Nodes.Add(id, name, iconKey, iconKey);
            }
            if (greyedOut)
            {
                newNode.ForeColor = Color.Gray;
            }
            ProjectTreeItem newItem = new ProjectTreeItem(id, newNode);
            newNode.Tag = newItem;
            return newItem;
        }

        public void AddTreeRoot(IEditorComponent plugin, string id, string name, string iconKey)
        {
            RegisterTreeNode(id, plugin);

            iconKey = SetDefaultIconIfNoneProvided(iconKey);
            _lastAddedNode = this._projectTree.Nodes.Add(id, name, iconKey, iconKey);

            ProjectTreeItem newItem = new ProjectTreeItem(id, _lastAddedNode);
            newItem.AllowLabelEdit = false;
            _lastAddedNode.Tag = newItem;
        }

        public void RemoveAllChildNodes(IEditorComponent plugin, string parentID)
        {
            TreeNode[] results = _projectTree.Nodes.Find(parentID, true);
            if (results.Length > 0)
            {
                results[0].Nodes.Clear();
            }
        }

        public void StartFromNode(IEditorComponent plugin, string id)
        {
            TreeNode[] results = _projectTree.Nodes.Find(id, true);
            if (results.Length > 0)
            {
                _lastAddedNode = results[0];
            }
            else
            {
                throw new AGSEditorException("Unable to select node " + id);
            }
        }

        public void ChangeNodeIcon(IEditorComponent plugin, string id, string newIconKey)
        {
            TreeNode[] results = _projectTree.Nodes.Find(id, true);
            if (results.Length > 0)
            {
                results[0].ImageKey = newIconKey;
                results[0].SelectedImageKey = newIconKey;
            }
        }

        public void SelectNode(IEditorComponent plugin, string id)
        {
            TreeNode[] results = _projectTree.Nodes.Find(id, true);
            if (results.Length > 0)
            {
                _projectTree.SelectedNode = results[0];
            }
        }

        public void ExpandNode(IEditorComponent plugin, string id)
        {
            TreeNode[] results = _projectTree.Nodes.Find(id, true);
            if (results.Length > 0)
            {
                results[0].Expand();
            }
        }

        public void BeginLabelEdit(IEditorComponent plugin, string nodeID)
        {
            SelectNode(plugin, nodeID);
            TreeNode[] results = _projectTree.Nodes.Find(nodeID, true);
            if (results.Length > 0)
            {
                results[0].BeginEdit();
            }
        }

        private string SetDefaultIconIfNoneProvided(string iconKey)
        {
            if ((iconKey == null) || (iconKey.Length == 0))
            {
                return DEFAULT_ICON_KEY;
            }
            return iconKey;
        }

        private void projectTree_MouseClick(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Right)
            {
                TreeNode node = _projectTree.HitTest(e.Location).Node;
                if (node != null)
                {
                    _projectTree.SelectedNode = node;
                    IList<MenuCommand> contextMenu = ProcessClickOnNode(node.Name, MouseButtons.Right);
                    if ((contextMenu != null) && (contextMenu.Count > 0))
                    {
                        ShowTreeContextMenu(contextMenu, e);
                    }
                }
            }
        }

        private void ContextMenuEventHandler(object sender, EventArgs e)
        {
            if (OnContextMenuClick != null)
            {
                ToolStripMenuItem item = (ToolStripMenuItem)sender;
                OnContextMenuClick(item.Name);
            }
        }

        private void ShowTreeContextMenu(IList<MenuCommand> commands, MouseEventArgs e)
        {
            EventHandler onClick = new EventHandler(ContextMenuEventHandler);
            ContextMenuStrip menu = new ContextMenuStrip();
            foreach (MenuCommand command in commands)
            {
                if (command.IsSeparator)
                {
                    menu.Items.Add(new ToolStripSeparator());
                }
                else
                {
                    Image icon = null;
                    if (command.IconKey != null)
                    {
                        icon = _projectTree.ImageList.Images[command.IconKey];
                    }
                    ToolStripMenuItem item = new ToolStripMenuItem(command.Name, icon, onClick, command.ID);
                    if (!command.Enabled)
                    {
                        item.Enabled = false;
                    }
                    item.Checked = command.Checked;
                    menu.Items.Add(item);
                }
            }

            menu.Show(_projectTree, e.Location);
        }

        private void AllowPluginsToModifyContextMenu(string nodeID, IList<MenuCommand> commands, IEditorComponent component)
        {
            if (BeforeShowContextMenu != null)
            {
                MenuCommands menuCommands = new MenuCommands(commands);
                BeforeShowContextMenu(new BeforeShowContextMenuEventArgs(nodeID, component, menuCommands));
                foreach (MenuCommand command in commands)
                {
                    if (!command.IsSeparator)
                    {
                        if (!Factory.GUIController.CanFindComponentFromMenuItemID(command.ID))
                        {
                            throw new AGSEditorException("A plugin or event handler has not used GUIController.CreateMenuCommand to create its menu command (ID '" + command.ID + "')");
                        }
                    }
                }
            }
        }

        private IList<MenuCommand> ProcessClickOnNode(string nodeID, MouseButtons button)
        {
            if (_treeNodes.ContainsKey(nodeID))
            {
                if (button == MouseButtons.Right)
                {
                    IEditorComponent plugin = _treeNodes[nodeID];
                    IList<MenuCommand> commands = plugin.GetContextMenu(nodeID);
                    if (commands != null)
                    {
                        Factory.GUIController.RegisterContextMenuCommands(commands, plugin);
                        AllowPluginsToModifyContextMenu(nodeID, commands, plugin);
                    }

                    return commands;
                }
                else
                {
                    _treeNodes[nodeID].CommandClick(nodeID);
                    return null;
                }
            }
            else
            {
                throw new AGSEditorException("Event fired from unknown tree node " + nodeID);
            }
        }

        private void projectTree_DoubleClick(object sender, EventArgs e)
        {
        }

        private void projectTree_AfterSelect(object sender, TreeViewEventArgs e)
        {
            _selectedNode = e.Node.Name;

            if (_treeNodes[_selectedNode] is IProjectTreeSingleClickHandler)
            {
                ((IProjectTreeSingleClickHandler)_treeNodes[_selectedNode]).SingleClick(_selectedNode);
            }
        }

        private bool HasANodeJustBeenExpanded()
        {
            return DateTime.Now.Subtract(_expandedAtTime) <= TimeSpan.FromMilliseconds(200);
        }

        private bool HasANodeBeenHoveredEnoughForExpanding()
        {
            return DateTime.Now.Subtract(_timeOfDragDropHoverStart) >= TimeSpan.FromMilliseconds(500);
        }

        private void projectTree_NodeMouseDoubleClick(object sender, TreeNodeMouseClickEventArgs e)
        {
            ProjectTreeItem treeItem = e.Node.Tag as ProjectTreeItem;
            bool acceptDoubleClickWhenExpanding = (treeItem != null && treeItem.AllowDoubleClickWhenExpanding);
            if (acceptDoubleClickWhenExpanding || !HasANodeJustBeenExpanded())
            {
                ProcessClickOnNode(e.Node.Name, MouseButtons.Left);
            }
        }

        private void projectTree_KeyPress(object sender, KeyPressEventArgs e)
        {
            if (e.KeyChar == (char)Keys.Return)
            {
                ProcessClickOnNode(_selectedNode, MouseButtons.Left);
            }
        }

        private void projectTree_BeforeLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            e.CancelEdit = true;
            if (e.Node.Tag != null)
            {
                ProjectTreeItem treeItem = ((ProjectTreeItem)e.Node.Tag);
                if (treeItem.AllowLabelEdit)
                {
                    e.CancelEdit = false;
                    // This hack is to allow the tree to display "5: Fifth view"
                    // but when you edit it, the user only edits "Fifth view"
                    string editableText = (string)treeItem.LabelTextProperty.GetValue(treeItem.LabelTextDataSource, null);
                    treeItem.LabelTextBeforeLabelEdit = editableText;
                    Hacks.SetTreeViewEditText(_projectTree, editableText);
                }
            }
        }

        private void projectTree_AfterLabelEdit(object sender, NodeLabelEditEventArgs e)
        {
            if ((e.Node.Tag != null) && (e.CancelEdit == false) && (e.Label != null))
            {
                ProjectTreeItem treeItem = (ProjectTreeItem)e.Node.Tag;
                try
                {
                    treeItem.LabelTextProperty.SetValue(treeItem.LabelTextDataSource, e.Label, null);
                    if (treeItem.LabelTextDescriptionProperty != null)
                    {
                        // This hack is so that if the user types "New view", the
                        // code can change it to "5: New view" for display in the tree
                        string newValue = (string)treeItem.LabelTextDescriptionProperty.GetValue(treeItem.LabelTextDataSource, null);
                        // cancel the user's entered text, and instead set it to our modified version
                        e.CancelEdit = true;
                        e.Node.Text = newValue;
                    }
                    if (OnAfterLabelEdit != null)
                    {
                        OnAfterLabelEdit(treeItem.ID, treeItem);
                    }
                }
                catch (Exception ex)
                {
                    if ((ex.InnerException != null) &&
                        (ex.InnerException is InvalidDataException))
                    {
                        Factory.GUIController.ShowMessage(ex.InnerException.Message, MessageBoxIcon.Warning);
                    }
                    else
                    {
                        throw new Exception(ex.Message, ex);
                    }
                    e.CancelEdit = true;
                }
            }
        }

        private void projectTree_ItemDrag(object sender, ItemDragEventArgs e)
        {
            TreeNode itemDragged = (TreeNode)e.Item;
            if (itemDragged.Tag != null)
            {
                ProjectTreeItem treeItem = (ProjectTreeItem)itemDragged.Tag;
                if (treeItem.AllowDragging)
                {
                    _projectTree.DoDragDrop(treeItem, DragDropEffects.Move);
                }
            }
        }

        private void HighlightNodeAndExpandIfNeeded(ProjectTreeItem item)
        {
            TreeNode treeNode = item.TreeNode;
            if (_treeNodesBackgroundColor == null)
            {
                _treeNodesBackgroundColor = treeNode.BackColor;
            }

            if (treeNode != _dropHoveredNode)
            {
                treeNode.BackColor = Color.LightGray;
                ClearHighlightNode();
                _dropHoveredNode = treeNode;
                _timeOfDragDropHoverStart = DateTime.Now;
            }
            else if (item.ExpandOnDragHover && HasANodeBeenHoveredEnoughForExpanding())
            {
                treeNode.Expand();
            }
        }

        private void ClearHighlightNode()
        {
            if (_dropHoveredNode != null)
            {
                _dropHoveredNode.BackColor = _treeNodesBackgroundColor.Value;
                _dropHoveredNode = null;
            }
        }

        private void projectTree_DragOver(object sender, DragEventArgs e)
        {
            e.Effect = DragDropEffects.None;

            if (e.Data.GetDataPresent(typeof(ProjectTreeItem)))
            {
                ProjectTreeItem source = (ProjectTreeItem)e.Data.GetData(typeof(ProjectTreeItem));
                Point locationInControl = _projectTree.PointToClient(new Point(e.X, e.Y));
                TreeNode dragTarget = _projectTree.HitTest(locationInControl).Node;
                if (dragTarget != null)
                {
                    ProjectTreeItem target = (ProjectTreeItem)dragTarget.Tag;
                    if (source.CanDropHere == null)
                    {
                        throw new AGSEditorException("Node has not populated CanDropHere handler for draggable node");
                    }
                    if (source.CanDropHere(source, target))
                    {
                        HighlightNodeAndExpandIfNeeded(target);
                        e.Effect = DragDropEffects.Move;
                    }
                    else ClearHighlightNode();

                    // auto-scroll the tree when move the mouse to top/bottom
                    if (locationInControl.Y < 30)
                    {
                        if (dragTarget.PrevVisibleNode != null)
                        {
                            dragTarget.PrevVisibleNode.EnsureVisible();
                        }
                    }
                    else if (locationInControl.Y > _projectTree.Height - 30)
                    {
                        if (dragTarget.NextVisibleNode != null)
                        {
                            dragTarget.NextVisibleNode.EnsureVisible();
                        }
                    }
                }
            }
        }

        private void projectTree_DragDrop(object sender, DragEventArgs e)
        {
            ClearHighlightNode();
            ProjectTreeItem source = (ProjectTreeItem)e.Data.GetData(typeof(ProjectTreeItem));
            Point locationInControl = _projectTree.PointToClient(new Point(e.X, e.Y));
            TreeNode dragTarget = _projectTree.HitTest(locationInControl).Node;
            ProjectTreeItem target = (ProjectTreeItem)dragTarget.Tag;

            if (source.DropHere == null)
            {
                throw new AGSEditorException("Node has not populated DropHere handler for draggable node");
            }

            source.DropHere(source, target);
        }

    }
}
