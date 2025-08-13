using AGS.Types;
using System;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    /// <summary>
    /// TreeViewWithDragDrop is an extended type of TreeView that handles drag and drop
    /// action for tree nodes. Supports dropping tree nodes on top of another tree node,
    /// or before or after particular node.
    /// It provides 3 new events that let customize its behavior:
    ///    * ItemTryDrag - called when the user starts dragging a tree node, and lets
    ///                    specify AllowedEffect (Move, Copy, etc), or cancel the action
    ///                    by setting DragDropEffects.None.
    ///    * ItemDragOver - called repeatedly while the tree node is being dragged around.
    ///    * ItemDragDrop - called when the tree node is dropped.
    /// </summary>
    public class TreeViewWithDragDrop : TreeView
    {
        public delegate void ItemTryDragEventHandler(object sender, TreeItemTryDragEventArgs e);
        public event ItemTryDragEventHandler ItemTryDrag;
        public delegate void ItemDragOverEventHandler(object sender, TreeItemDragEventArgs e);
        public event ItemDragOverEventHandler ItemDragOver;
        public delegate void ItemDragDropEventHandler(object sender, TreeItemDragEventArgs e);
        public event ItemDragDropEventHandler ItemDragDrop;
        public delegate bool PriorityKeyDownEventHandler(object sender, KeyEventArgs e);
        public event PriorityKeyDownEventHandler PriorityKeyDown;

        // Time to wait while dragging cursor hovers over a node before expanding it
        private const int DragWaitBeforeExpandNodeMs = 500;

        private TreeNode _dropHoveredNode;
        private Color _treeNodeBackColor = Color.Empty;
        private Color _treeNodeForeColor = Color.Empty;
        // TODO: public properties to configure highlight colors? may be useful with color themes
        // NOTE: if the treeview supports both dragging its own item, and dragging something else
        // from another place, then highlight colors will change depending on a case.
        private Color _treeNodeHighlightBackColor = SystemColors.Highlight;
        private Color _treeNodeHighlightForeColor = SystemColors.HighlightText;
        private DateTime _timeOfDragDropHoverStart;
        private LineInBetween _lineInBetween;

        public TreeViewWithDragDrop()
        {
            _lineInBetween = new LineInBetween();
            Controls.Add(_lineInBetween);
            _lineInBetween.BringToFront();
            _lineInBetween.Hide();
        }


        protected override bool ProcessCmdKey(ref Message msg, Keys keyData)
        {
            if(this.Focused)
            {
                return HandlePriorityKeyDown(keyData);
            }

            return base.ProcessCmdKey(ref msg, keyData);
        }

        private bool HandlePriorityKeyDown(Keys keyData)
        {
            if (PriorityKeyDown != null)
            {
                foreach (PriorityKeyDownEventHandler handler in PriorityKeyDown.GetInvocationList())
                { 
                    if (handler.Invoke(this, new KeyEventArgs(keyData)))
                        return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Tells if the dragging cursor was hovering above another node long enough to expand it.
        /// </summary>
        private bool HasANodeBeenHoveredEnoughForExpanding()
        {
            return DateTime.Now.Subtract(_timeOfDragDropHoverStart) >= TimeSpan.FromMilliseconds(DragWaitBeforeExpandNodeMs);
        }

        /// <summary>
        /// Reacts to dragging cursor hovering over another node: highlights that node,
        /// and optionally expands it, if it's expandable.
        /// </summary>
        private void HighlightNodeAndExpandIfNeeded(TreeNode treeNode, bool expandOnDragHover, TargetDropZone dropZone)
        {
            if (treeNode != _dropHoveredNode)
            {
                ClearHighlightNode();
                // Save normal colors of the hovered over node, and set highlight colors
                _treeNodeBackColor = treeNode.BackColor;
                _treeNodeForeColor = treeNode.ForeColor;
                _dropHoveredNode = treeNode;
                _dropHoveredNode.BackColor = _treeNodeHighlightBackColor;
                _dropHoveredNode.ForeColor = _treeNodeHighlightForeColor;
                _timeOfDragDropHoverStart = DateTime.Now;
                if (_dropHoveredNode == SelectedNode)
                    HideSelection = true;
            }
            else if (expandOnDragHover && HasANodeBeenHoveredEnoughForExpanding() && dropZone == TargetDropZone.Middle)
            {
                treeNode.Expand();
            }
        }

        /// <summary>
        /// Clears highlight from the last highlighted tree node.
        /// </summary>
        private void ClearHighlightNode()
        {
            if (_dropHoveredNode != null)
            {
                _dropHoveredNode.BackColor = _treeNodeBackColor;
                _dropHoveredNode.ForeColor = _treeNodeForeColor;
                if (_dropHoveredNode == SelectedNode)
                    HideSelection = false;
                _dropHoveredNode = null;
            }
        }

        /// <summary>
        /// Displays the "in-between tree nodes" marker at the given coordinates.
        /// </summary>
        private void ShowMiddleLine(int x, int y, int w, int h)
        {
            // it auto-hides so we don't have to handle the drop being cancelled which has to be done in projectItem!
            _lineInBetween.ShowAndHideAt(x, y, w, h);
        }

        /// <summary>
        /// Hide thes "in-between tree nodes" marker.
        /// </summary>
        private void HideMiddleLine()
        {
            _lineInBetween.Hide();
        }

        /// <summary>
        /// Calculates the necessary width of a "in-between tree nodes" marker,
        /// depending on a selected tree node.
        /// </summary>
        private int GetLineInBetweenWidth(TreeNode treeNode)
        {
            int maxWdith = treeNode.Bounds.Width;
            if (treeNode.PrevNode != null)
            {
                maxWdith = Math.Max(maxWdith, treeNode.PrevNode.Bounds.Width);
            }
            if (treeNode.NextNode != null)
            {
                maxWdith = Math.Max(maxWdith, treeNode.NextNode.Bounds.Width);
            }
            return Math.Max(maxWdith, this.Width / 3);
        }

        /// <summary>
        /// Gets the location type of a cursor relative to the tree node.
        /// </summary>
        private TargetDropZone GetDropZoneImpl(int y, int h)
        {
            TargetDropZone dropZone = TargetDropZone.Middle;

            if (y <= h / 4)
            {
                dropZone = TargetDropZone.Top;
            }
            else if (y > h / 4 && y < 2 * h / 5)
            {
                dropZone = TargetDropZone.MiddleTop;
            }
            else if (y > 3 * h / 5 && y < 3 * h / 4)
            {
                dropZone = TargetDropZone.MiddleBottom;
            }
            else if (y >= 3 * h / 4)
            {
                dropZone = TargetDropZone.Bottom;
            }

            return dropZone;
        }

        /// <summary>
        /// Gets the location type of a cursor relative to the tree node.
        /// </summary>
        private TargetDropZone GetDropZone(TreeNode treeNode, Point locationInControl)
        {
            int node_h = treeNode.Bounds.Height;
            int node_y = treeNode.Bounds.Y;
            int cur_y = locationInControl.Y;
            return GetDropZoneImpl(cur_y - node_y, node_h);
        }

        /// <summary>
        /// Clears all visual drag highlights and markers.
        /// </summary>
        private void ClearAllDragHighlights()
        {
            HideMiddleLine();
            ClearHighlightNode();
        }

        /// <summary>
        /// Completely resets the visual state of a drag'n'drop action,
        /// including any graphical configuration prepared when the drag starts.
        /// </summary>
        private void ResetDragDropState()
        {
            ClearAllDragHighlights();
            // Reset highlight color setup back to defaults
            _treeNodeHighlightBackColor = SystemColors.Highlight;
            _treeNodeHighlightForeColor = SystemColors.HighlightText;
        }

        protected override void OnQueryContinueDrag(QueryContinueDragEventArgs e)
        {
            if (e.EscapePressed)
            {
                e.Action = DragAction.Cancel;
            }

            if (e.Action != DragAction.Continue)
            {
                ResetDragDropState();
            }
        }

        protected override void OnDragDrop(DragEventArgs e)
        {
            ResetDragDropState();
            // If not dragging a tree item, then fallback to standard drag drop
            if (!e.Data.GetDataPresent(typeof(TreeNode)))
            {
                base.OnDragDrop(e);
                return;
            }

            TreeNode dragItem = (TreeNode)e.Data.GetData(typeof(TreeNode));
            Point locationInControl = PointToClient(new Point(e.X, e.Y));
            TreeNode dropTarget = HitTest(locationInControl).Node;
            TargetDropZone dropZone = GetDropZone(dropTarget, locationInControl);

            OnItemDragDrop(new TreeItemDragEventArgs(e, dragItem, dropTarget, dropZone, false, false));
        }

        protected override void OnDragEnter(DragEventArgs e)
        {
            base.OnDragEnter(e);
        }

        protected override void OnDragLeave(EventArgs e)
        {
            base.OnDragLeave(e);
            ClearAllDragHighlights();
        }

        protected override void OnDragOver(DragEventArgs e)
        {
            // If not dragging a tree item, then let them override a standard drag drop
            bool isDraggingItem = e.Data.GetDataPresent(typeof(TreeNode));
            if (!isDraggingItem)
            {
                base.OnDragOver(e);

                if (e.Effect == DragDropEffects.None)
                    return; // was cancelled
            }

            Point locationInControl = PointToClient(new Point(e.X, e.Y));
            TreeNode dropTarget = HitTest(locationInControl).Node;
            TargetDropZone dropZone = TargetDropZone.Middle;
            bool showLine = false;
            bool expandOnHover = true;

            // If dragging an item, invoke ItemDragOver event and let subscribers adjust highlight parameters
            if (isDraggingItem)
            {
                TreeNode dragItem = (TreeNode)e.Data.GetData(typeof(TreeNode));
                dropZone = GetDropZone(dropTarget, locationInControl);
                showLine = dropZone == TargetDropZone.Top || dropZone == TargetDropZone.Bottom;
                TreeItemDragEventArgs itemDrag = new TreeItemDragEventArgs(e, dragItem, dropTarget, dropZone, showLine, expandOnHover);
                OnItemDragOver(itemDrag); // let user code respond to the drag state
                e.Effect = itemDrag.Effect;
                showLine = itemDrag.ShowLine;
                expandOnHover = itemDrag.ExpandOnDragHover;

                // Safety check: dragging a tree node into the sub-nodes is forbidden
                if (e.Effect != DragDropEffects.None && dropTarget.IsDescendantOf(dragItem))
                {
                    e.Effect = DragDropEffects.None;
                }
            }

            // Apply visual indication, depending on Effect and position
            if (e.Effect != DragDropEffects.None)
            {
                int node_h = dropTarget.Bounds.Height;
                int node_y = dropTarget.Bounds.Y;
                int line_h = node_h / 5;
                int width = GetLineInBetweenWidth(dropTarget);

                if (dropZone == TargetDropZone.Top && showLine)
                {
                    ShowMiddleLine(dropTarget.Bounds.X, node_y - line_h / 2, width, line_h);
                }
                else if (dropZone == TargetDropZone.Bottom && showLine)
                {
                    ShowMiddleLine(dropTarget.Bounds.X, node_y + node_h - line_h / 2, width, line_h);
                }
                else
                {
                    HideMiddleLine();
                }

                HighlightNodeAndExpandIfNeeded(dropTarget, expandOnHover, dropZone);
            }
            else
            {
                ClearAllDragHighlights();
            }

            // auto-scroll the tree when move the mouse to top/bottom
            if (locationInControl.Y < 30)
            {
                if (dropTarget.PrevVisibleNode != null)
                {
                    dropTarget.PrevVisibleNode.EnsureVisible();
                }
            }
            else if (locationInControl.Y > Height - 30)
            {
                if (dropTarget.NextVisibleNode != null)
                {
                    dropTarget.NextVisibleNode.EnsureVisible();
                }
            }
        }

        protected override void OnItemDrag(ItemDragEventArgs e)
        {
            var tryDrag = new TreeItemTryDragEventArgs(e);
            OnItemTryDrag(tryDrag);

            if (tryDrag.AllowedEffect == DragDropEffects.None)
                return;

            // Because we drag a selected item, we use a different color for highlighting drop target
            _treeNodeHighlightBackColor = Color.LightGray;
            _treeNodeHighlightForeColor = Color.Empty;
            DoDragDrop(e.Item, tryDrag.AllowedEffect);
            base.OnItemDrag(e);
        }

        protected virtual void OnItemTryDrag(TreeItemTryDragEventArgs e)
        {
            ItemTryDrag?.Invoke(this, e);
        }

        protected virtual void OnItemDragOver(TreeItemDragEventArgs e)
        {
            ItemDragOver?.Invoke(this, e);
        }

        protected virtual void OnItemDragDrop(TreeItemDragEventArgs e)
        {
            ItemDragDrop?.Invoke(this, e);
        }
    }
}
