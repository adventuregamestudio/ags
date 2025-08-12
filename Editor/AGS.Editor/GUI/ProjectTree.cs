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
        private TreeViewWithDragDrop _projectTree;
        private TreeNode _lastAddedNode = null;
		private DateTime _expandedAtTime = DateTime.MinValue;
        private string _selectedNode;


        public ProjectTree(TreeViewWithDragDrop projectTree)
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
            _projectTree.KeyDown += new System.Windows.Forms.KeyEventHandler(this.projectTree_KeyDown);
            _projectTree.BeforeLabelEdit += new System.Windows.Forms.NodeLabelEditEventHandler(this.projectTree_BeforeLabelEdit);
			_projectTree.BeforeExpand += new TreeViewCancelEventHandler(_projectTree_BeforeExpand);
			_projectTree.BeforeCollapse += new TreeViewCancelEventHandler(_projectTree_BeforeCollapse);
			_projectTree.ItemTryDrag += projectTree_ItemTryDrag;
            _projectTree.ItemDragOver += _projectTree_ItemDragOver;
            _projectTree.ItemDragDrop += _projectTree_ItemDragDrop;
        }

        private void _projectTree_BeforeCollapse(object sender, TreeViewCancelEventArgs e)
		{
            _expandedAtTime = DateTime.Now;
		}

		private void _projectTree_BeforeExpand(object sender, TreeViewCancelEventArgs e)
		{
            _expandedAtTime = DateTime.Now;
		}

        public void BeginUpdate()
        {
            _projectTree.BeginUpdate();
        }

        public void EndUpdate()
        {
            _projectTree.EndUpdate();
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
            TreeNodeCollection nodes = (_lastAddedNode != null) ? _lastAddedNode.Nodes : this._projectTree.Nodes;
            
            // only audiocomponent requires manual arrangement, and only in the main folder
            bool foldersOnTop = !(component is Components.AudioComponent && _lastAddedNode != null && _lastAddedNode.Level == 0);

            if (foldersOnTop && iconKey == "GenericFolderIcon")
            {
                // find last folder index so we can insert right below it, but above other entries
                // speech icon canonically stays above
                int index = 0;
                
                for (; index < nodes.Count;index++)
                {
                    if (nodes[index].ImageKey != iconKey) break;
                }
                _lastAddedNode = nodes.Insert(index, id, name, iconKey, iconKey);
            }
            else
            {
                _lastAddedNode = nodes.Add(id, name, iconKey, iconKey);
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
            TreeNode node = _projectTree.Nodes.FindUnique(parentID, true);
            if (node != null)
            {
                node.Nodes.Clear();
            }
        }

        public void StartFromNode(IEditorComponent plugin, string id)
        {
            TreeNode node = _projectTree.Nodes.FindUnique(id, true);
            if (node != null)
            {
                _lastAddedNode = node;
            }
            else
            {
                throw new AGSEditorException("Unable to select node " + id);
            }
        }

        public void ChangeNodeLabel(IEditorComponent plugin, string id, string newLabelText)
        {
            TreeNode node = _projectTree.Nodes.FindUnique(id, true);
            if (node != null)
            {
                node.Text = newLabelText;
            }
        }

        public void ChangeNodeIcon(IEditorComponent plugin, string id, string newIconKey)
        {
            TreeNode node = _projectTree.Nodes.FindUnique(id, true);
            if (node != null)
            {
                node.ImageKey = newIconKey;
                node.SelectedImageKey = newIconKey;
            }
        }

        public void SelectNode(IEditorComponent plugin, string id)
        {
            TreeNode node = _projectTree.Nodes.FindUnique(id, true);
            if (node != null)
            {
                _projectTree.SelectedNode = node;
            }
        }

        public void ExpandNode(IEditorComponent plugin, string id)
        {
            TreeNode node = _projectTree.Nodes.FindUnique(id, true);
            if (node != null)
            {
                node.Expand();
            }
        }

        /// <summary>
        /// Returns a list of currently expanded tree nodes.
        /// </summary>
        public List<string> GetExpansionState()
        {
            return _projectTree.Nodes.GetExpansionState(
                    (node) => { return node.Name; });
        }

        /// <summary>
        /// Applies expanded state to the tree nodes which match given paths.
        /// </summary>
        public void SetExpansionState(List<string> state)
        {
            _projectTree.Nodes.SetExpansionState(state,
                (node) => { return state.Contains(node.Name); } );
        }

        public void BeginLabelEdit(IEditorComponent plugin, string nodeID)
        {
            SelectNode(plugin, nodeID);
            TreeNode node = _projectTree.Nodes.FindUnique(nodeID, true);
            if (node != null)
            {
                node.BeginEdit();
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
                    ToolStripMenuItem item = ToolStripExtensions.CreateMenuItem(command.Name, icon, onClick, command.ID, command.ShortcutKey);
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

        private void projectTree_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.F2)
            {
                if (_projectTree.SelectedNode != null)
                    _projectTree.SelectedNode.BeginEdit();
            }

            // We are going to pass down keys in case a context menu entry has shortcuts
            if (_projectTree.SelectedNode != null)
            {
                TreeNode node = _projectTree.SelectedNode;
                string nodeId = node.Name;

                if (_treeNodes.ContainsKey(nodeId))
                {
                    IEditorComponent component = _treeNodes[nodeId];
                    IList<MenuCommand> contextCommands = component.GetContextMenu(nodeId);
                    if (contextCommands != null)
                    {
                        foreach (MenuCommand command in contextCommands)
                        {
                            if (command.IsSeparator) continue;
                            if (command.ShortcutKey == e.KeyData && command.Enabled)
                            {
                                component.CommandClick(command.ID);
                            }
                        }
                    }
                }
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

        #region Drag and Drop

        private void projectTree_ItemTryDrag(object sender, TreeItemTryDragEventArgs e)
		{
			TreeNode itemDragged = (TreeNode)e.Item;
            if ((itemDragged.Tag != null) && ((ProjectTreeItem)itemDragged.Tag).AllowDragging)
            {
                e.AllowedEffect = DragDropEffects.Move;
            }
            else
            {
                e.AllowedEffect = DragDropEffects.None;
            }
		}

        private void _projectTree_ItemDragOver(object sender, TreeItemDragEventArgs e)
        {
            ProjectTreeItem source = (ProjectTreeItem)e.DragItem.Tag;
            ProjectTreeItem target = (ProjectTreeItem)e.DropTarget.Tag;

            bool showLine = false;
            if (source.CanDropHere != null && source.CanDropHere(source, target, e.DropZone, out showLine))
            {
                e.Effect = DragDropEffects.Move;
                e.ShowLine = showLine;
                e.ExpandOnDragHover = target.ExpandOnDragHover;
            }
            else
            {
                e.Effect = DragDropEffects.None;
                e.ShowLine = false;
                e.ExpandOnDragHover = false;
            }
        }

        private void _projectTree_ItemDragDrop(object sender, TreeItemDragEventArgs e)
        {
            ProjectTreeItem source = (ProjectTreeItem)e.DragItem.Tag;
            ProjectTreeItem target = (ProjectTreeItem)e.DropTarget.Tag;
            source?.DropHere(source, target, e.DropZone);
        }

        #endregion // Drag and Drop
    }
}
