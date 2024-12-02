using AGS.Types;
using System;
using System.Drawing;
using System.Windows.Forms;

namespace AGS.Editor
{
    public class TreeViewWithDragDrop : TreeView
    {
        public delegate void ItemTryDragEventHandler(object sender, TreeItemTryDragEventArgs e);
        public event ItemTryDragEventHandler ItemTryDrag;
        public delegate void ItemDragOverEventHandler(object sender, TreeItemDragEventArgs e);
        public event ItemDragOverEventHandler ItemDragOver;
        public delegate void ItemDragDropEventHandler(object sender, TreeItemDragEventArgs e);
        public event ItemDragDropEventHandler ItemDragDrop;

        // Time to wait while dragging cursor hovers over a node before expanding it
        private const int DragWaitBeforeExpandNodeMs = 500;

        private Color? _treeNodesBackgroundColor;
        private TreeNode _dropHoveredNode;
        private DateTime _timeOfDragDropHoverStart;
        private LineInBetween _lineInBetween;

        public TreeViewWithDragDrop()
        {
            _lineInBetween = new LineInBetween();
            Controls.Add(_lineInBetween);
            _lineInBetween.BringToFront();
            _lineInBetween.Hide();
        }

        private bool HasANodeBeenHoveredEnoughForExpanding()
        {
            return DateTime.Now.Subtract(_timeOfDragDropHoverStart) >= TimeSpan.FromMilliseconds(DragWaitBeforeExpandNodeMs);
        }

        private void HighlightNodeAndExpandIfNeeded(TreeNode treeNode, bool expandOnDragHover, TargetDropZone dropZone)
        {
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
            else if (expandOnDragHover && HasANodeBeenHoveredEnoughForExpanding() && dropZone == TargetDropZone.Middle)
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

        private void ShowMiddleLine(int x, int y, int w, int h)
        {
            // it auto-hides so we don't have to handle the drop being cancelled which has to be done in projectItem!
            _lineInBetween.ShowAndHideAt(x, y, w, h);
        }

        private void HideMiddleLine()
        {
            _lineInBetween.Hide();
        }

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

        private TargetDropZone GetDropZone(TreeNode treeNode, Point locationInControl)
        {
            int node_h = treeNode.Bounds.Height;
            int node_y = treeNode.Bounds.Y;
            int cur_y = locationInControl.Y;
            return GetDropZoneImpl(cur_y - node_y, node_h);
        }

        protected override void OnQueryContinueDrag(QueryContinueDragEventArgs e)
        {
            if (e.EscapePressed)
            {
                e.Action = DragAction.Cancel;
            }

            if (e.Action != DragAction.Continue)
            {
                HideMiddleLine();
            }
        }

        protected override void OnDragDrop(DragEventArgs e)
        {
            // If not dragging a tree item, then fallback to standard drag drop
            if (!e.Data.GetDataPresent(typeof(TreeNode)))
            {
                base.OnDragDrop(e);
                return;
            }

            TreeNode dragItem = (TreeNode)e.Data.GetData(typeof(TreeNode));
            HideMiddleLine();
            ClearHighlightNode();
            Point locationInControl = PointToClient(new Point(e.X, e.Y));
            TreeNode dropTarget = HitTest(locationInControl).Node;
            TargetDropZone dropZone = GetDropZone(dropTarget, locationInControl);

            OnItemDragDrop(new TreeItemDragEventArgs(e, dragItem, dropTarget, dropZone));
        }

        protected override void OnDragEnter(DragEventArgs e)
        {
            base.OnDragEnter(e);
        }

        protected override void OnDragLeave(EventArgs e)
        {
            base.OnDragLeave(e);
        }

        protected override void OnDragOver(DragEventArgs e)
        {
            // If not dragging a tree item, then fallback to standard drag drop
            if (!e.Data.GetDataPresent(typeof(TreeNode)))
            {
                base.OnDragOver(e);
                return;
            }

            TreeNode dragItem = (TreeNode)e.Data.GetData(typeof(TreeNode));
            Point locationInControl = PointToClient(new Point(e.X, e.Y));
            TreeNode dropTarget = HitTest(locationInControl).Node;
            if (dropTarget == null)
            {
                e.Effect = DragDropEffects.None; // cancel
                return;
            }

            TargetDropZone dropZone = GetDropZone(dropTarget, locationInControl);
            TreeItemDragEventArgs itemDrag = new TreeItemDragEventArgs(e, dragItem, dropTarget, dropZone);
            OnItemDragOver(itemDrag); // let user code respond to the drag state
            e.Effect = itemDrag.Effect;

            if (itemDrag.Effect != DragDropEffects.None)
            {
                int node_h = dropTarget.Bounds.Height;
                int node_y = dropTarget.Bounds.Y;
                int line_h = node_h / 5;
                int width = GetLineInBetweenWidth(dropTarget);

                if (dropZone == TargetDropZone.Top && itemDrag.ShowLine)
                {
                    ShowMiddleLine(dropTarget.Bounds.X, node_y - line_h / 2, width, line_h);
                }
                else if (dropZone == TargetDropZone.Bottom && itemDrag.ShowLine)
                {
                    ShowMiddleLine(dropTarget.Bounds.X, node_y + node_h - line_h / 2, width, line_h);
                }
                else
                {
                    HideMiddleLine();
                }

                HighlightNodeAndExpandIfNeeded(dropTarget, itemDrag.ExpandOnDragHover, dropZone);
            }
            else
            {
                HideMiddleLine();
                ClearHighlightNode();
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
